/*
  Caviscope_CAM_3.ino
  - ESP32-CAM (AI-Thinker)
  - ILI9341 2" TFT via software SPI (CS, DC, RST, MOSI, SCK)
  - Display rendered image from SPIFFS (/photo.jpg) using TJpg_Decoder
  - OTA, Wi-Fi (via WiFi_Manager), buzzer feedback
  - No MISO used
*/

char * devicename = "Caviscope_CAM_on_BG_Screen";

#include <Arduino.h>
#include "WiFi.h"
#include "esp_camera.h"
#include "esp_timer.h"
#include "img_converters.h"
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "driver/rtc_io.h"
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include <FS.h>

#include "credentials.h"    // your Wi-Fi list
#include "buzzer.h"
#include "camera_pins.h"    // your camera pin defines (Y2.. etc)
#include "WiFi_Manager.h"

#include <SPI.h>
#include <Adafruit_GFX.h>
//#include <Adafruit_ST7735.h>
#include <Adafruit_ST7789.h>

#include <TJpg_Decoder.h>
#include <ArduinoOTA.h>

#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>

// ---------------- TFT (software SPI) pins (no MISO) ----------------
#define TFT_CS    2   // Chip Select
#define TFT_DC    12  // Data/Command
#define TFT_RST   14  // Reset
#define TFT_MOSI  13  // MOSI (DIN)
#define TFT_CLK   15  // SCLK

// Adafruit ST77XX constructor (software SPI)
Adafruit_ST7789 LCD = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_MOSI, TFT_CLK, TFT_RST);

// ---------------- Globals & config ----------------
Buzzer buzzer(4);                    // buzzer pin
WiFi_Manager wifi_obj(0);            // your WiFi helper (keeps your existing interface)

AsyncWebServer server(80);
const char* FILE_PHOTO = "/photo.jpg";

bool takeNewPhoto = false;
bool wifiConnected = false;
uint8_t err = 0;
char error_logged[120] = ".";
char image_location[40] = "...";
char wifi_network[64] = "No Network";
bool successfully_captured = false;

const unsigned long captureIntervalMs = 2UL * 60UL * 1000UL; // 1 minute
uint64_t lastShot = 0;
unsigned long lastCaptureTime = 0;
const unsigned long captureCooldown = 3000; // ms

// ---------------- Camera config helper (AI Thinker assumed) ----------------
void configure_camera(){
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

  Serial.printf("PSRAM_FOUND = %d  Free PSRAM: %u  FREE HEAP = %u\n", psramFound(), ESP.getFreePsram(), ESP.getFreeHeap());

  if (psramFound()) {
    // You indicated you want HD but keep images small for OTA. tune quality to cap size.
    config.frame_size = FRAMESIZE_HD;    // 1280x720
    config.jpeg_quality = 12;            // 10..14 is a good compromise
    config.fb_count = 1;
    config.fb_location = CAMERA_FB_IN_PSRAM;
  } else {
    config.frame_size = FRAMESIZE_QVGA;  // 320x240
    config.jpeg_quality = 16;
    config.fb_count = 1;
  }

  esp_err_t rc = esp_camera_init(&config);
  if (rc != ESP_OK) {
    Serial.printf("Camera init failed: 0x%x\n", rc);
    err = 13;
    // show on LCD then restart
    delay(2000);
    ESP.restart();
  }
}

// ---------------- TJpg_Decoder callback ----------------
// Signature matches the library expectation
bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t *bitmap) {
  // The TJpg decoder hands us 16-bit RGB565 pixel blocks.
  // Adafruit_ST77XX expects data as uint16_t and has pushColors.
  // If colors look swapped, change the 'true' swap flag below or do byte swap here.
  LCD.startWrite();
  LCD.setAddrWindow(x, y, w, h);
  // pushColors expects a pointer to 16-bit data and count; set third param true to swap bytes if necessary.
 //  LCD.pushColor(bitmap, w * h, true); // 'true' swaps bytes if needed (fixes many color-order issues)
  for (uint32_t i = 0; i < w * h; i++) {
    LCD.pushColor(bitmap[i]);
  }


  
  LCD.endWrite();
  return true;
}

// ---------------- Filesystem helpers ----------------
bool ensureSPIFFS() {
  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS mount failed!");
    return false;
  }
  return true;
}

bool checkPhoto(fs::FS &fs) {
  File f = fs.open(FILE_PHOTO);
  if (!f) return false;
  size_t s = f.size();
  f.close();
  return (s > 10); // slightly larger than nothing
}

// ---------------- Capture to SPIFFS ----------------
void capturePhotoSaveSpiffs() {
  // cooldown
  if (millis() - lastCaptureTime < captureCooldown) return;

  // ensure some space (attempt simple housekeeping)
  size_t available = SPIFFS.totalBytes() - SPIFFS.usedBytes();
  // minimal heuristic: if less than 50k, try removing previous image
  if (available < 50UL * 1024UL) {
    SPIFFS.remove(FILE_PHOTO);
    delay(10);
    available = SPIFFS.totalBytes() - SPIFFS.usedBytes();
    if (available < 50UL * 1024UL) {
      err = 18; // insufficient space
      error_log(err);
      return;
    }
  }

  camera_fb_t * fb = esp_camera_fb_get();
  if (!fb) {
    err = 1;
    error_log(err);
    return;
  }

  File f = SPIFFS.open(FILE_PHOTO, FILE_WRITE);
  if (!f) {
    err = 2;
    esp_camera_fb_return(fb);
    error_log(err);
    return;
  }

  size_t written = f.write(fb->buf, fb->len);
  f.close();
  esp_camera_fb_return(fb);

  if (written != fb->len) {
    err = 19;
    error_log(err);
    return;
  }

  // verify
  if (checkPhoto(SPIFFS)) {
    err = 5; // success
    successfully_captured = true;
    lastCaptureTime = millis();
    Serial.printf("Picture saved (%u bytes)\n", (unsigned)written);
  } else {
    err = 4;
    successfully_captured = false;
  }
  error_log(err);
}

// ---------------- Render image from SPIFFS onto TFT ----------------
void fetchAndRenderImage() {
  // show small loading indicator (optional)
  LCD.fillRect(0, 0, 40, 16, ST77XX_BLACK);
  LCD.setCursor(2, 2);
  LCD.setTextColor(ST77XX_WHITE);
  LCD.setTextSize(1);
  LCD.print("Rendering...");

  // TJpg_Decoder can read directly from a filename in SPIFFS if library version supports it
  // Use drawJpg with filename (const char*) as many TJpg_Decoder builds offer.
  // If your TJpg_Decoder does not support FS path, you would instead read file into buffer and call drawJpg(buffer,len).
  JRESULT jres = TJpgDec.drawJpg(0, 0, FILE_PHOTO);
  if (jres == JDR_OK) {
    Serial.println("Rendered image from SPIFFS");
  } else {
    Serial.printf("TJpg decode returned: %d\n", (int)jres);
    // fallback: attempt buffered decode
    File f = SPIFFS.open(FILE_PHOTO, FILE_READ);
    if (f) {
      size_t len = f.size();
      uint8_t *buf = (uint8_t*)malloc(len);
      if (buf) {
        f.read(buf, len);
        f.close();
        TJpgDec.drawJpg(0, 0, buf, len);
        free(buf);
      } else {
        Serial.println("Buffer alloc failed for fallback decode.");
      }
    } else {
      Serial.println("File open failed for fallback decode.");
    }
  }

  // remove loading text
  LCD.fillRect(0, 0, 40, 16, ST77XX_BLACK);
}

// ---------------- UI helpers (camera icon, search bar, check box, status) ----------------
void draw_camera_2(int camX, int camY);
void draw_search_bar(int x, int y, int w, int h, const char* text);
void draw_check_box(int x, int y, bool checked, bool yellowTick);

void LCD_status(const char* text) {
  // simple single-screen status area
  LCD.fillRect(0, 0, 320, 40, ST77XX_WHITE); 
  LCD.setFont();
  LCD.setTextSize(1);
  LCD.setTextColor(ST77XX_BLUE);
  if (wifiConnected) {
    if (!successfully_captured) draw_camera_2(210, 10);
    else draw_camera_2(230, 10);
    draw_search_bar(8, 6, 150, 26, image_location);
    LCD.setCursor(8, 44);
    LCD.setTextColor(ST77XX_GREEN);
    LCD.print("Network: "); LCD.print(wifi_network);
  } else {
    LCD.setCursor(8, 8);
    LCD.print("WiFi: disconnected");
  }

  LCD.setCursor(8, 70);
  LCD.setTextColor(ST77XX_BLACK);
  LCD.print(text);
}

void draw_camera_2(int camX, int camY) {
  int camW = 40; int camH = 25;
  LCD.fillRoundRect(camX + 2, camY + 2, camW, camH, 4, ST77XX_RED);
  LCD.fillRoundRect(camX, camY, camW, camH, 4, ST77XX_CYAN);
  LCD.fillRoundRect(camX + 10, camY - 5, 22, 10, 3, ST77XX_CYAN);
  int lensX = camX + camW/2; int lensY = camY + camH/2;
  LCD.fillCircle(lensX, lensY, 7, ST77XX_BLACK);
  LCD.fillCircle(lensX, lensY, 4, ST77XX_RED);
  LCD.fillCircle(lensX, lensY, 3, ST77XX_YELLOW);
  LCD.fillCircle(camX + camW - 5, camY + 6, 3, ST77XX_WHITE);
  LCD.drawCircle(camX + camW - 5, camY + 6, 3, ST77XX_BLACK);
}

void draw_search_bar(int x, int y, int w, int h, const char* text) {
  //LCD.fillRoundRect(x + 2, y + 2, w, h, 8, ST77XX_DARKGREY);
  LCD.fillRoundRect(x, y, w, h, 8, ST77XX_BLACK);
  int lensX = x + 10; int lensY = y + h/2;
  LCD.drawCircle(lensX, lensY, 5, ST77XX_WHITE);
  LCD.drawLine(lensX + 3, lensY + 3, lensX + 8, lensY + 8, ST77XX_WHITE);
  LCD.setTextColor(ST77XX_WHITE);
  LCD.setCursor(lensX + 15, y + (h/2)-3);
  LCD.print(text);
}

void draw_check_box(int x, int y, bool checked = true, bool yellowTick = false) {
  LCD.fillRoundRect(x, y, 20, 20, 3, ST77XX_GREEN);
  LCD.drawRoundRect(x, y, 20, 20, 3, ST77XX_BLACK);
  if (checked) {
    uint16_t tc = yellowTick ? ST77XX_YELLOW : ST77XX_WHITE;
    LCD.drawLine(x+5,y+10, x+9,y+15, tc);
    LCD.drawLine(x+9,y+15, x+16,y+5, tc);
  }
}

// ---------------- Error logger ----------------
void error_log(int log_count) {
  switch (log_count) {
    case 0: strcpy(error_logged, "Camera on standby..."); break;
    case 1: strcpy(error_logged, "Camera Capture Failed"); successfully_captured = false; break;
    case 2: strcpy(error_logged, "File Open Failed!"); successfully_captured = false; break;
    case 4: strcpy(error_logged, "SPIFFS Failed!!!"); successfully_captured = false; break;
    case 5: {
      File f = SPIFFS.open(FILE_PHOTO);
      if (f) {
        float k = f.size() / 1024.0;
        snprintf(error_logged, sizeof(error_logged), "Photo Taken | %.1f kB", k);
        f.close();
        successfully_captured = true;
      } else {
        strcpy(error_logged, "Photo saved (size unknown)");
      }
      break;
    }
    case 10: strcpy(error_logged, "SPIFFS mount failed"); break;
    case 11: strcpy(error_logged, "Scanning Wi-Fi..."); break;
    case 12: strcpy(error_logged, "No known Wi-Fi found"); break;
    case 13: strcpy(error_logged, "Camera init failed"); break;
    case 14:
      strncpy(wifi_network, WiFi.SSID().c_str(), sizeof(wifi_network)-1);
      strncpy(image_location, WiFi.localIP().toString().c_str(), sizeof(image_location)-1);
      snprintf(error_logged, sizeof(error_logged), "%s Connected | IP: %s", wifi_network, image_location);
      break;
    case 15: strcpy(error_logged, "Wi-Fi connect failed"); break;
    case 16:
      strncpy(error_logged, "Camera on Standby", sizeof(error_logged)); break;
    case 17: strcpy(error_logged, "Capturing photo..."); break;
    case 18: snprintf(error_logged, sizeof(error_logged), "Insufficient Space"); break;
    case 19: snprintf(error_logged, sizeof(error_logged), "Write Mismatch!"); break;
    default: strcpy(error_logged, "Unknown State."); break;
  }

  Serial.println(error_logged);
  LCD_status(error_logged);
}


void initializeOTA(){
  // OTA
  ArduinoOTA.setHostname(devicename);
  ArduinoOTA.onStart([](){ Serial.println("OTA Start"); });
  ArduinoOTA.onEnd([](){ Serial.println("\nOTA End"); });
  ArduinoOTA.onProgress([](unsigned int p, unsigned int t){ Serial.printf("OTA %u%%\r", (p*100)/t); });
  ArduinoOTA.onError([](ota_error_t e){ Serial.printf("OTA Error %u\n", e); });
  ArduinoOTA.begin();
  Serial.println("OTA ready");
}

// ---------------- Setup ----------------
void setup() {
  Serial.begin(115200);
  delay(100);
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

  // TFT init
 // LCD.begin();
  LCD.init(320, 480);
  LCD.setRotation(1);
  LCD.fillScreen(ST77XX_RED); delay(2000); // R
  LCD.fillScreen(ST77XX_GREEN); delay(2000); // G
  LCD.fillScreen(ST77XX_BLUE); delay(2000); // B

  LCD.fillScreen(ST77XX_WHITE);

  
    LCD.setTextWrap(true);
    LCD.setTextColor(ST77XX_BLACK);

  // Backlight if you have a pin for it, else ignore
  // pinMode(BACKLIGHT_PIN, OUTPUT); digitalWrite(BACKLIGHT_PIN, HIGH);

  // boot animation / message
   // BootScreen();

  // begin filesystem
  if (!ensureSPIFFS()) {
    err = 10; error_log(err);
    delay(1000); ESP.restart();
  }

  // buzzer init
    buzzer.begin();
    // CORE TASK 1
      xTaskCreatePinnedToCore(FastTask, "BuzzingCheck", 2048, NULL, 1, NULL, 1);
      Serial.println("\tBuzzer Task initialized!");
      
      buzzer.beep(1,50,0); // now this can go off in exactly 50ms before the loop comes in


  // wifi: use your WiFi_Manager helper (you had this previously)
  err = 11; error_log(err);
  wifi_obj.initialize_ESP_WiFi(devicename);
  wifiConnected = (WiFi.status() == WL_CONNECTED);
  if (wifiConnected) {
    err = 14; error_log(err);
  } else {
    err = 15; error_log(err);
    // you said only for OTA; continue but OTA needs WiFi
  }

  initializeOTA();

    // camera
  configure_camera();


  // TJpg init
  TJpgDec.setCallback(tft_output);
  TJpgDec.setJpgScale(1);  // try 1; increase to 2 if you hit memory limits

  // initial UI
  LCD.fillScreen(ST77XX_WHITE);
  LCD_status("Ready");
  delay(500);
}

// like checking on boiling milk few times every second
void FastTask(void * pvParams) { 
  uint8_t checkin_frequency = 10; // ~10ms
 
  const TickType_t xDelay = pdMS_TO_TICKS(checkin_frequency);
  while (1) {
    buzzer.update(); // OFF if duration etuuse // maybe also buttons.update();
    
    vTaskDelay(xDelay); // yield to la skedula
  }

}



// ---------------- Main loop ----------------
uint64_t last_wifi_check = 0;
void loop() {
  // capture every minute
  if ((esp_timer_get_time() - lastShot) >= (int64_t)captureIntervalMs * 1000) { // once every 2 minutes
    takeNewPhoto = true;
    lastShot = esp_timer_get_time();
  }

  if (takeNewPhoto) {
    err = 17; error_log(err);
   // digitalWrite(torch, HIGH); delay(400);
   // buzzer.beep(1, 40, 0);

    capturePhotoSaveSpiffs();

    // render the new picture (if saved)
    if (successfully_captured) {
      fetchAndRenderImage();
      // small visual acknowledgement
      draw_check_box(10, 200, true, true);
    }

    takeNewPhoto = false;
   // digitalWrite(torch, LOW);
  }

  // periodic wifi ensure (lightweight helper)
  if ((esp_timer_get_time() - last_wifi_check) >= 30LL * 1000LL * 1000LL) { // every 30 seconds
        // try to re-init via helper (non-blocking ideally)
      wifi_obj.ensure_wifi();
      if((WiFi.status() != WL_CONNECTED)) {err = 15; error_log(err);} // this shouldn't be here since it is in the helper
        last_wifi_check = esp_timer_get_time();
    }
  

  ArduinoOTA.handle();
  delay(1); // not too long that it would block OTA uploads
}

// ---------------- End of file ----------------
