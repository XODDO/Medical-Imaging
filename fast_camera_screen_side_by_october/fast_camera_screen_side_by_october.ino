#define devicename "Caviscope_CAM_2"
#include "WiFi.h"
#include "esp_camera.h"
#include "esp_timer.h"
#include "img_converters.h"
#include "Arduino.h"
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "driver/rtc_io.h"
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include <FS.h>
#include "credentials.h"    // Your Wi-Fi list
#include <webpage2.h>
#include <buzzer.h>
#include <camera_pins.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>
#include <ArduinoOTA.h>

#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>


// ====== TFT PINS ======
#define TFT_CS    2
#define TFT_RST   14
#define TFT_DC    12
#define TFT_MOSI  13
#define TFT_SCLK  15

Adafruit_ST7789 LCD = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);
Buzzer buzzer(4);   // camera inbuilt flash is also on 4!!!   // buzzer pin

uint8_t flash = 1; // UOT :: SERIAL TX

// ====== GLOBALS ======
AsyncWebServer server(80);
boolean takeNewPhoto = false;
unsigned long lastWiFiAttempt = 0;
const unsigned long reconnectInterval = 15000;
bool wifiConnected = false;
uint8_t err = 0;
char error_logged[120] = ".";
char DEVICE_ID[40] = "CAVI-SCOPE";

// ====== Function prototypes ======
void capturePhotoSaveSpiffs(void);
bool checkPhoto(fs::FS &fs);
void error_log(int log_count);
bool connectKnownWiFi(void);
void configure_camera();
void LCD_header(const char* text);
void LCD_status(const char* text);

#define IMAGE_FILE "/photo.jpg"

char image_location[25] = "..."; // stores current IP
float image_size_kb = 0.0f;
bool ip_addr_ready = false;
char wifi_network[64] = "No Network"; // from 0-63 characters
bool successfully_captured = false;

// ====== SETUP ======
void setup() { delay(5);
   // Serial.begin(115200); delay(500);  // Serial.println("Booting...");// taken by LCD DC_PIN
    
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); // disable brownout

  
    LCD.init(76, 284);
    LCD.setRotation(1);
    LCD.fillScreen(ST77XX_BLACK);
    LCD.setTextWrap(true);
    LCD.setTextColor(ST77XX_BLACK);
    LCD_header(DEVICE_ID); delay(2000);
    LCD_status("Initializing..."); delay(1000);
  
    pinMode(flash, OUTPUT);
    digitalWrite(flash, HIGH);

    configure_camera();

  
  if (!SPIFFS.begin(true)) {
      err = 10;
      error_log(err);
      delay(1000);
      ESP.restart(); // SOFT RESTART
  }

  err = 25; // chech storage ish
  error_log(err);
 
  delay(2000);

  err = 11;
  error_log(err);
  wifiConnected = connectKnownWiFi();
  if (!wifiConnected) {
      err = 12;
      error_log(err);
      delay(2000);
      ESP.restart();
  }

  // cannot proceed if wifi is off

  // Web server routes
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
      request->send_P(200, "text/html", index_html);
  });
  
  server.on("/capture", HTTP_GET, [](AsyncWebServerRequest *request) {
      takeNewPhoto = true;
      request->send(200, "text/plain", "Capturing photo...");
      err = 17;
  });
  
  // Add these routes to your ESP32 code:
      server.on("/photo.jpg", HTTP_GET, [](AsyncWebServerRequest *request) {
          request->send(SPIFFS, IMAGE_FILE, "image/jpg", false);
      });

      server.on("/placeholder.jpg", HTTP_GET, [](AsyncWebServerRequest *request) {
          request->send(SPIFFS, "/placeholder.jpg", "image/jpg", false);
      });

      // Keep your existing routes
      server.on("/photo-in-spiffs", HTTP_GET, [](AsyncWebServerRequest *request) {
          request->send(SPIFFS, IMAGE_FILE, "image/jpg", false);
      });

    if(wifiConnected){
      server.begin();
      err = 16; // only after WiFi has successfully connected
      error_log(err);
    }

    buzzer.begin();
    buzzer.beep(3, 50, 50);
    digitalWrite(flash, LOW);


 // Hostname defaults to esp3232-[MAC]
   ArduinoOTA.setHostname(devicename); 

  // No authentication by default
 //   ArduinoOTA.setPassword("admin");

  // and maybe set password with pre-hashed value (SHA256 hash of "admin")
//   SHA256(admin) = 8c6976e5b5410415bde908bd4dee15dfb167a9c873fc4bb8a81f6f2ab448a918
 //  ArduinoOTA.setPasswordHash("8c6976e5b5410415bde908bd4dee15dfb167a9c873fc4bb8a81f6f2ab448a918");

     ArduinoOTA
    .onStart([]() {
      Serial.println("Start updating...");
    })
    .onEnd([]() {
      Serial.println("\nUpdate complete!");
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress * 100) / total);
    })
    .onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });

  ArduinoOTA.begin();
  Serial.println("OTA Ready!");

 //  savePlaceholderToSPIFFS(); < 1.5KB


}

// ====== CAMERA CONFIG ======
void configure_camera() {
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  Serial.printf("PSRAM_FOUND = %d\nFree PSRAM: %u\nFREE HEAP = %u\n", psramFound(), ESP.getFreePsram(), ESP.getFreeHeap());

  if (psramFound()) {
   // LOG("PSRAM detected  — using SVGA.");
    config.frame_size =  FRAMESIZE_HD; //FRAMESIZE_HD HD = 1280x720 (~200kB), SGVA = 800x600 (100kB), UXGA = 1600x1200 (~300kB), FRAMESIZE_QVGA = 320x240
    config.jpeg_quality = 12; // 12 is recommended: lower is higher quality, takes up more memory
    config.fb_count = 1; // CRITICAL With 190kB, you simply don't have room for multiple buffers plus image storage.
    config.fb_location = CAMERA_FB_IN_PSRAM;
  } else {
    // LOG("No PSRAM  — using smaller frame.");
    config.frame_size = FRAMESIZE_QVGA; // QVGA = 320x240 1-5kB
    config.jpeg_quality = 20;
    config.fb_count = 1;
  }

  esp_err_t err_cam = esp_camera_init(&config);
  if (err_cam != ESP_OK) {
    err = 13;
    error_log(err);
    delay(2000);
    ESP.restart();
  }
}

/*
//older but responsive index_html
const char index_html[] PROGMEM
R"rawliteral(
      <!DOCTYPE HTML><html>
      <head><meta name="viewport" content="width=device-width, initial-scale=1">
      <style>body{text-align:center;font-family:sans-serif;}</style></head>
      <body>
      <h2>ESP32-CAM Laparoscope Capture</h2>
      <p><button onclick="capturePhoto()">CAPTURE</button></p>
      <img src="saved-photo" id="photo" width="70%">
      <script>
        function capturePhoto(){fetch('/capture').then(()=>setTimeout(()=>location.reload(),3000));}
      </script></body></html>)rawliteral"
*/
bool flash_is_on = false;
bool otaEnabled = true;

// == TAKE A PIC, SHARE AND BEEP == //
void loop() {
  if (takeNewPhoto) {
      digitalWrite(flash, HIGH); vTaskDelay(pdMS_TO_TICKS(500)); flash_is_on = true; //  For the flash light to be ON
      capturePhotoSaveSpiffs();
      error_log(err);
      takeNewPhoto = false;
      buzzer.beep(1, 50, 0);
   }  else { 
      err = 0; buzzer.update(); 
      if(flash_is_on) {
        digitalWrite(flash, LOW); // CAN USE DIRECT PORT ACCESS TO ACCELERATE FROM MICROSECONDS TO NANOSECSS
        flash_is_on = false;
      }
    }

  if (WiFi.status() != WL_CONNECTED && millis() - (lastWiFiAttempt > reconnectInterval)) {
      err = 16;
      error_log(err);
      wifiConnected = connectKnownWiFi();
      lastWiFiAttempt = millis();
  }
  vTaskDelay(pdMS_TO_TICKS(5));

  if (otaEnabled) ArduinoOTA.handle();
}



// ====== CAPTURE PHOTO ======
bool checkPhoto(fs::FS &fs) {
  File picture_file = fs.open(IMAGE_FILE);

  if(!picture_file) {
     err = 20; // Failed to Open
     picture_file.close();
    return false;
  }
  
  if (picture_file.size() < 10) {  // Pic cannot be less than 10Bytes
     err = 21; // Way too small
     picture_file.close(); 
     return false; 
    }

  picture_file.close();

    // if we get here, ...  successful save, proceed to open the pic

  return true;
}

unsigned long lastCaptureTime = 0;
const unsigned long captureCooldown = 3000; // ms
size_t availableSpace;
size_t bytesWritten;
void capturePhotoSaveSpiffs() {
  if (millis() - lastCaptureTime < captureCooldown) return;      // Skip if cooldown not over
    
   // Critical: Check if we have enough space
  availableSpace = SPIFFS.totalBytes() - SPIFFS.usedBytes();
  
  if (availableSpace < 52000) { // a pic needs atleast 50kB  free 
        // First remove an old file first to free space
        SPIFFS.remove(IMAGE_FILE);
        
        // Small delay to let SPIFFS cleanup
        delay(10);

         // Re-check available space after cleanup
    availableSpace = SPIFFS.totalBytes() - SPIFFS.usedBytes();
    if (availableSpace < 52000) {
        err = 18; // Still insufficient space after cleanup
        return; // CRITICAL: Return here!
    }
  }
  
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) { err = 1; return; }

  File file = SPIFFS.open(IMAGE_FILE, FILE_WRITE);
  if (!file) { err = 2; esp_camera_fb_return(fb); return; }

   size_t bytesWritten = file.write(fb->buf, fb->len);
   file.close();
   esp_camera_fb_return(fb);

     // Better verification
    if (bytesWritten != fb->len) {
      err = 19; // size mismatch
      
      return;
    }

  bool photo_verified = checkPhoto(SPIFFS);
  if (photo_verified) err = 5;
  //else err =  4; //  the earlier errors: 21 and 22 should take precendence over the vague SPIFFS Failed!

  lastCaptureTime = millis(); // reset cooldown
  
}


// ====== ERROR LOGGER ======
void error_log(int log_count) {
  switch (log_count) {
    case 0:
      strcpy(error_logged, "Camera on standby...");
      break;
    case 1:
      strcpy(error_logged, "Camera Capture Failed"); successfully_captured = false;
      break;
    case 2:
      strcpy(error_logged, "File Open Failed!"); successfully_captured = false;
      break;
    case 4:
      strcpy(error_logged, "SPIFFS Failed!!!"); successfully_captured = false;
      break;
    case 5: { // Photo taken and successfully saved to SPIFFS
      File f = SPIFFS.open(IMAGE_FILE);
      if (f) {
        image_size_kb = f.size() / 1024.0;  // size in kB
        snprintf(error_logged, sizeof(error_logged),
                 "Photo Taken | %.1f kB", image_size_kb);
                 
        f.close(); successfully_captured = true;
      } else {
        strcpy(error_logged, "Photo saved (size unknown)");
      }
      break;
    }
    case 10:
      strcpy(error_logged, "SPIFFS mount failed");
      break;
    case 11:
      strcpy(error_logged, "Scanning Wi-Fi...");
      break;
    case 12:
      strcpy(error_logged, "No known Wi-Fi found");
      break;
    case 13:
      strcpy(error_logged, "Camera init failed");
      break;

    case 14:  // Wi-Fi connected
      strncpy(wifi_network, WiFi.SSID().c_str(), sizeof(wifi_network)); image_location[sizeof(wifi_network)-1] = 0; // ensure null termination
      strncpy(image_location, WiFi.localIP().toString().c_str(), sizeof(image_location)); image_location[sizeof(image_location)-1] = 0; // ensure null termination

      snprintf(error_logged, sizeof(error_logged),
               "%s Connected | IP: %s",
                wifi_network, image_location);     
      break;

    case 15: // failed WiFi
    strcpy(error_logged, "Wi-Fi connect failed");
       
      break;

    case 16: // Web server started
     // snprintf(error_logged, sizeof(error_logged),  "Cav Camera is Ready", image_location);
      strncpy(error_logged,  "Camera on Standby", sizeof(error_logged));
      snprintf(image_location, sizeof(image_location),  "http://%s", WiFi.localIP().toString().c_str());
      
      break;
    case 17: // successfully taken a shot
      strcpy(error_logged, "Capturing photo...");
      break;

    case 18:
      snprintf(error_logged, sizeof(error_logged), "Insufficient Space: %d", availableSpace);
     break;

    case 19:
       snprintf(error_logged, sizeof(error_logged), "Write MisMatch! %d", bytesWritten);
      break;

    case 20:
      snprintf(error_logged, sizeof(error_logged), "Failed to Open!");
    break;

    case 21:
        snprintf(error_logged, sizeof(error_logged), "File too tiny!!!!");
    break;

    case 25: {
     static unsigned long lastLog = 0;
       // if (millis() - lastLog > 30000) { // Every 30 seconds
              size_t total = (SPIFFS.totalBytes()/1024.0);
              size_t used = (SPIFFS.usedBytes()/1024.0);
              
              lastLog = millis();
        
            snprintf(error_logged, sizeof(error_logged), "[SPIFFS]: %.2f/%.2f kB (%d%%)\n", 
                        used, total, (used * 100) / total);
           // }
         }
        break;

    default:
      strcpy(error_logged, "Unknown State.");
      break;
  }

  Serial.println(error_logged);
  LCD_status(error_logged);
}



// ====== LCD Helper Functions ======
void LCD_header(const char* text) {
  LCD.fillScreen(ST77XX_BLACK);
  LCD.setFont(&FreeSansBold12pt7b);
  LCD.setCursor(60, 50);
  LCD.setTextColor(ST77XX_WHITE);
  LCD.setTextWrap(true);
  LCD.print(text);
}

void LCD_status(const char* text) {
  LCD.fillScreen(ST77XX_WHITE);
  LCD.setFont(&FreeSans9pt7b);
  LCD.setTextColor(ST77XX_BLUE);
  LCD.setTextWrap(true);

  if(wifiConnected) { //  when camera is ready
    //camera icon
    if(!successfully_captured) draw_camera_2(210, 30);
    else draw_camera_2(230, 30);

    draw_search_bar(25, 10, 175, 20, image_location);

    //LCD.setCursor(60, 30);
    //LCD.print(image_location);


    LCD.setTextColor(ST77XX_GREEN);
    LCD.setFont();
    LCD.setCursor(50, 60);
    LCD.print("Network: "); LCD.print(wifi_network);
  

    LCD.setFont(&FreeSans9pt7b); // for the next print outside the if
    LCD.setTextColor(ST77XX_BLUE); 
    LCD.setCursor(20, 50); 
  } else LCD.setCursor(20, 40); // otherwise it center
  
  LCD.print(text);
}

void draw_check_box(int x, int y, bool checked = true, bool yellowTick = false) {
  // --- Outer box ---
  LCD.fillRoundRect(x, y, 20, 20, 3, ST77XX_GREEN);     // green background
  LCD.drawRoundRect(x, y, 20, 20, 3, ST77XX_BLACK);     // outline for definition

  if (checked) {
    uint16_t tickColor = yellowTick ? ST77XX_YELLOW : ST77XX_WHITE;

    // --- Draw the tick mark ---
    LCD.drawLine(x + 5,  y + 10, x + 9,  y + 15, tickColor); // lower-left to mid
    LCD.drawLine(x + 9,  y + 15, x + 16, y + 5,  tickColor); // mid to upper-right
  }
}


void draw_search_bar(int x, int y, int w, int h, const char* text) {
  // --- Shadow (soft offset) ---
  LCD.fillRoundRect(x + 2, y + 2, w, h, 10, ST77XX_BLACK);

  // --- Search bar background ---
  LCD.fillRoundRect(x, y, w, h, 10, ST77XX_BLACK);

  // --- Search lens icon (optional) ---
  int lensX = x + 10;
  int lensY = y + h / 2;
  LCD.drawCircle(lensX, lensY, 5, ST77XX_WHITE);   // lens ring
  LCD.drawLine(lensX + 3, lensY + 3, lensX + 8, lensY + 8, ST77XX_WHITE); // handle

  // --- Text inside search bar ---
  LCD.setTextColor(ST77XX_WHITE);
 // LCD.setFont();
  LCD.setFont(&FreeSans9pt7b);
  LCD.setTextSize(1);

  int textX = lensX + 15;  // offset to avoid overlapping the lens
  int textY = y + (h / 2) - 3;
  LCD.setCursor(textX, textY+10);
  LCD.print(text);
}


void draw_camera(int starting_x, int starting_y){
        LCD.fillRoundRect((starting_x+30), (starting_x-5), 20, 10, 4, ST77XX_CYAN); // the head of the camera
        LCD.fillRoundRect(starting_x, starting_y, 40, 30, 8, ST77XX_CYAN); // main body
        LCD.fillCircle(240, 40, 8, ST77XX_BLACK); // the lens
        //LCD.fillTriangle(122, 10, 142, 60, 190, 40, ST77XX_RED);
}


void draw_camera_2(int camX, int camY) {
  int camW = 40;  // body width
  int camH = 25;  // body height

  // --- Draw soft shadow (offset slightly right & down)
  LCD.fillRoundRect(camX + 2, camY + 2, camW, camH, 4, ST77XX_ORANGE);

  // --- Camera body ---
  LCD.fillRoundRect(camX, camY, camW, camH, 4, ST77XX_CYAN);

  // --- Camera top/head ---
  LCD.fillRoundRect(camX + 10, camY - 5, 22, 10, 3, ST77XX_CYAN);

  // --- Lens ---
  int lensX = camX + camW / 2;
  int lensY = camY + camH / 2;
  LCD.fillCircle(lensX, lensY, 7, ST77XX_BLACK);      // lens border
  LCD.fillCircle(lensX, lensY, 4, ST77XX_RED);  // softer transition ring
  LCD.fillCircle(lensX, lensY, 3, ST77XX_YELLOW);     // lens highlight

  // --- Flash ---
  LCD.fillCircle(camX + camW - 5, camY + 6, 3, ST77XX_WHITE);
  LCD.drawCircle(camX + camW - 5, camY + 6, 3, ST77XX_BLACK); // subtle outline
}



// ====== Wi-Fi Connect Cycle ======
bool connectKnownWiFi() {
  bool connected_state = false;
  
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(true);
  delay(100);

  int n = WiFi.scanNetworks();
  if (n <= 0) return false;

  int bestIndex = -1, bestRSSI = -999;
  for (int i = 0; i < n; i++) {
    String foundSSID = WiFi.SSID(i);
    int32_t rssi = WiFi.RSSI(i);
    for (int j = 0; j < knownCount; j++) {
      if (foundSSID == knownNetworks[j].SECRET_SSID) {
        if (rssi > bestRSSI) { bestRSSI = rssi; bestIndex = j; }
      }
    }
  }

  if (bestIndex == -1) return false;

  WiFi.begin(knownNetworks[bestIndex].SECRET_SSID, knownNetworks[bestIndex].SECRET_PASS);
  unsigned long startAttempt = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startAttempt < 10000) {
    delay(100);
    yield();
  }

  if (WiFi.status() == WL_CONNECTED) {
    err = 14;
    error_log(err);
    connected_state = true;
   // return true;
  } else { // WiFi connection failed
    err = 15;
    error_log(err);
    connected_state = false;
   // return false;
  }

  return connected_state;
}




