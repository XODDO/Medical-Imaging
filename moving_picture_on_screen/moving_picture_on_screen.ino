/*
  Caviscope_CAM_ILI9341_Video.ino
  ---------------------------------------------------
  - ESP32-CAM (AI Thinker)
  - 2.8" TFT (ILI9341, hardware SPI)
  - Real-time video streaming from camera
  - Optimized for 320x240 display
  - Includes buzzer + on-screen feedback
*/

char *devicename = "Caviscope_Vid_CAM";
char *OTA_PASS = "1234";

#include <Arduino.h>
#include "esp_camera.h"
#include "esp_timer.h"
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "driver/rtc_io.h"
#include <SPIFFS.h>
#include <FS.h>
#include "buzzer.h"
#include "camera_pins.h"


#include "credentials.h"
#include "WiFi_Manager.h"
#include <ArduinoOTA.h>
#include "SPI.h"
#include <TJpg_Decoder.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>


#include "Fonts/FreeSans9pt7b.h"
#include "Fonts/FreeSans12pt7b.h"
#include "Fonts/FreeSans18pt7b.h"

#include <Fonts/FreeSansBold9pt7b.h> 
#include <Fonts/FreeSansBold12pt7b.h> 
#include <Fonts/FreeSansBold18pt7b.h> 

#include <Fonts/FreeMono9pt7b.h>
#include <Fonts/FreeMonoBold9pt7b.h>

#include <Fonts/FreeMono12pt7b.h>
#include <Fonts/FreeMonoBold12pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include <Fonts/FreeSansBold9pt7b.h>

#define SCREEN_MOSI 13 
#define SCREEN_SCK 15
#define SCREEN_MISO -1 // NOT NEEDED
#define SCREEN_CS 2

/* Pins 13, 14, 15 are connected to the ESP32's internal flash memory on ESP32-CAM. Using them for SPI can cause conflicts.*/

// ---------- TFT Pins ----------
#define TFT_MOSI 16
#define TFT_CLK  14 
#define TFT_CS   12   // GPIO12 - Might work with SD card disabled
#define TFT_DC   2   // GPIO2 - Boot pin, handle carefully
#define TFT_RST  15 // 15 might demand a pulldown resistor to be low at boot
#define TFT_LED  -4   //  backlight is connected to Vcc through a 120ohm res
#define TFT_MISO -2  // no need to read anything to the screen

uint8_t flash = 4;

Buzzer buzzer(15);  //  LED pin if no dedicated buzzer pin

// Create custom SPI instance on HSPI with your pin remapping
SPIClass* TFT_SPI = NULL;

// Declare LCD pointer - we'll initialize it after SPI is set up
//Adafruit_ILI9341* LCD = NULL;

//SPIClass TFT_SPI(HSPI);


// ---------- TFT ----------
  Adafruit_ILI9341 LCD = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST); // hardware spi
//Adafruit_ILI9341 LCD = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_MOSI, TFT_CLK, TFT_RST); // this constructs images very slowly
// Customize the Adafruit_ILI9341 constructor to use hardware SPI


// ---------- Video Streaming Globals ----------
bool isStreaming = true;
bool successfully_captured = false;
uint32_t frameCounter = 0;
uint32_t lastFpsTime = 0;
float fps = 0;

// For optional still capture during streaming
bool takeNewPhoto = false;
const char *FILE_PHOTO = "/photo.jpg";
const unsigned long captureCooldown = 3000;
unsigned long lastCaptureTime = 0;
uint64_t lastShot = 0;
const unsigned long captureIntervalMs = 1 * 60UL * 1000UL;  // Optional: auto-capture every minute

// Buffer for direct frame processing
uint8_t *jpgBuffer = NULL;
size_t jpgBufferSize = 0;

// ---------- Forward Declarations ----------
void BuzzingTask(void *pvParams);
void configure_camera();
bool ensureSPIFFS();
bool checkPhoto(fs::FS &fs);
void capturePhotoSaveSpiffs();
void streamVideo();
void displayFrame(camera_fb_t *fb);
bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t *bitmap);
void error_log(const char *msg, uint16_t color = ILI9341_YELLOW);
void updateFPS();
void displayStats();

bool wifi_connected = false;
uint32_t WiFi_Strength = 0;
char ota_log[150] = "...";
volatile bool otaFinished = false;
volatile bool otaStarted = false;
volatile bool otaError = false;

WiFi_Manager wifi_obj(27); // pin 2

uint64_t now_now_ms = 0;

// ----------------------------------------------------------
// Setup
// ----------------------------------------------------------
void setup() {
  delay(100);
    // ====== STEP 1: Handle strapping pins FIRST ======

// GPIO0 must be LOW at boot for normal operation
  pinMode(0, INPUT_PULLDOWN);  // If using as TFT_RST
  delay(10);
  
  // GPIO2 also needs care
  pinMode(2, INPUT_PULLDOWN);
  delay(10);
  
  // GPIO12 if SD card not used
  pinMode(12, INPUT_PULLDOWN);
  delay(10);
  
  // GPIO15 if not using, keep it LOW
  pinMode(15, INPUT_PULLDOWN);
  delay(10);

//Serial.begin(115200);
   // ====== CRITICAL: Fix strapping/flash pins BEFORE anything else ======
  // GPIO14 is connected to flash - set as input before using as TFT_RST
  //pinMode(14, INPUT); //  TFT_RST
  //delay(10);

 // pinMode(0, INPUT_PULLDOWN); // TFT_CLK at boot

  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
  delay(500);
  // Optional: backlight ON
  //pinMode(TFT_LED, OUTPUT);
  //digitalWrite(TFT_LED, HIGH);

 /*
  // TFT Init
   // Initialize custom SPI first
  TFT_SPI.begin(TFT_CLK, TFT_MISO, TFT_MOSI, TFT_CS);
  TFT_SPI.setFrequency(26000000); // 26MHz
  
  
  // Then initialize display

 // Initialize the custom SPI instance
  TFT_SPI = new SPIClass(HSPI);
  
  // Begin SPI with custom pins BEFORE initializing the display
  TFT_SPI->begin(TFT_CLK, TFT_MISO, TFT_MOSI, TFT_CS);
  
  // Set SPI frequency for optimal performance
  // ILI9341 can handle up to ~40MHz, but 26-30MHz is safe for most wiring
  TFT_SPI->setFrequency(26000000);  // 26MHz
  
  // You can also set SPI mode (ILI9341 uses mode 0)
  TFT_SPI->setDataMode(SPI_MODE0);
  /*
  // Set bit order (MSB first is standard)
  TFT_SPI->setBitOrder(MSBFIRST);
  
  // Initialize the LCD with the custom SPI instance
  LCD.setSPI(*TFT_SPI);
  */

  //  LCD.begin(26000000);


  // Buzzer
  buzzer.begin();
  xTaskCreatePinnedToCore(BuzzingTask, "BuzzingCheck", 4096, NULL, 1, NULL, 1);
  delay(100);
  buzzer.beep(1,50,0);


  // ====== STEP 2: Initialize camera BEFORE SPI ======
  Serial.println("Initializing camera FIRST...");
  configure_camera();
  Serial.println("Camera initialized successfully!");
  
  // ====== STEP 3: Now initialize SPI and TFT ======
  Serial.println("Initializing SPI...");
  
 
 // Configure hardware SPI globally
  SPI.begin(TFT_CLK, TFT_MISO, TFT_MOSI, TFT_CS);
  SPI.setFrequency(26000000);  // Max speed
  
  // Initialize display - it will use the hardware SPI even though we used software constructor
  LCD.begin();
  
  // Override the SPI speed
  LCD.setSPISpeed(26000000);

  //LCD.begin();
  LCD.setRotation(3);
  LCD.fillScreen(ILI9341_NAVY);

   LCD.setFont(&FreeSansBold18pt7b);
    LCD.setCursor(5, 40); LCD.setTextColor(ILI9341_YELLOW);
    LCD.print("MADE IN UGANDA");
    delay(1000);
    LCD.setFont(); LCD.setTextColor(ILI9341_WHITE);
    LCD.setCursor(40, 60); LCD.print("Running Flag Speed-Test..."); 
  
 // UGANDA FLAG SPEED TEST
  uint32_t test_start = millis();
  
     for(int i=0; i<=320; i+=2){
        LCD.fillRect(0, 180, i, 20, ILI9341_BLACK);
       } 

    for(int i=0; i<=320; i+=2){
      LCD.fillRect(0, 200, i, 20, ILI9341_YELLOW);
    }  
    
    for(int i=0; i<=320; i+=2){
      LCD.fillRect(0, 220, i, 20, ILI9341_RED);
    } 
    
    uint32_t test_stop = millis();

    float test_duration = ((float)test_stop - (float)test_start)/1000.0; // convert to seconds

    LCD.setFont(&FreeSans12pt7b);
    LCD.setCursor(5, 130); LCD.setTextColor(ILI9341_CYAN);
    LCD.print("Test Duration: "); LCD.print(test_duration); LCD.print(" seconds");


    delay(5000);
    LCD.fillScreen(ILI9341_BLACK);
    LCD.setTextColor(ILI9341_GREEN);
    LCD.setFont(&FreeSans12pt7b);
    LCD.setTextSize(1);
    LCD.setCursor(20, 20); LCD.println("Caviscope Video Device");
  //  LCD.drawHLine
    
    LCD.setFont(&FreeSans9pt7b);
     LCD.setCursor(40, 50); LCD.println("Initializing...");
     delay(1000);

  // SPIFFS (optional, for still capture)
  if (!ensureSPIFFS()) {
    error_log("SPIFFS mount failed!", ILI9341_RED);
    delay(2000);
    // Continue without SPIFFS for video streaming
  }

  LCD.setCursor(20, 100); LCD.println("> File System Initialized!");
  delay(1000);

  // Camera - Configure for fastest capture
  configure_camera();
  LCD.setCursor(20, 130); LCD.println("> Camera Initialized!");
  delay(1000);


  // JPEG Decoder for streaming
   TJpgDec.setCallback(tft_output);
 // TJpgDec.setJpgScale(1);  // Full resolution for better quality video
  LCD.setCursor(20, 160); LCD.println("> JPG CallBack Set!");
  delay(1000);

  // Optimize TJpgDec settings
   TJpgDec.setJpgScale(0);  // No scaling for fastest decode
   TJpgDec.setSwapBytes(false);  // Match your display's byte order

  // Allocate buffer for JPEG decoding
  jpgBufferSize = 1024 * 32;  // 32KB buffer for JPEG chunks
  jpgBuffer = (uint8_t*)malloc(jpgBufferSize);
  LCD.setCursor(20, 190); LCD.println("> Buffer Size Set to 32kB!");
  delay(1000);


  if (!jpgBuffer) {
    error_log("Buffer alloc failed!", ILI9341_RED);
    delay(2000);
    ESP.restart();
  }

  LCD.fillScreen(ILI9341_BLACK); 
  LCD.setFont(&FreeSans12pt7b);

  //error_log("", ILI9341_GREEN);
  LCD.setCursor(10, 100); LCD.println("Video Streaming Ready!");
  delay(1000);


  // Start video streaming immediately
  isStreaming = true;

  now_now_ms = esp_timer_get_time()/1000ULL;
  initialize_radio_on_wifi();
  //initializeOTA();
      LCD.setFont(&FreeSans9pt7b);

    buzzer.beep(2,50,50);

}

// ----------------------------------------------------------
// Main Loop - Video Streaming
// ----------------------------------------------------------
void loop() {
  now_now_ms = esp_timer_get_time()/1000ULL;
  // Optional: Check for auto-capture trigger
  if ((now_now_ms - lastShot) >= (int64_t)captureIntervalMs * 1000) {
    takeNewPhoto = true;
    lastShot = now_now_ms;
  }

  // Handle still capture if triggered
  if (takeNewPhoto) {
    isStreaming = false;
    error_log("Capturing still...", ILI9341_YELLOW);
    capturePhotoSaveSpiffs();
    
    if (successfully_captured) {
      buzzer.beep(1, 50, 0);
      
      // Display the captured still
      File f = SPIFFS.open(FILE_PHOTO, "r");
      size_t picSize = f.size();
      f.close();
      
      LCD.fillScreen(ILI9341_BLACK);
      TJpgDec.drawJpg(0, 0, FILE_PHOTO);
      
      // Display file size
      LCD.fillRect(0, 210, 320, 30, ILI9341_BLACK);
      LCD.setTextColor(ILI9341_GREEN);
      LCD.setCursor(10, 220);
      LCD.setTextSize(2);
      LCD.printf("Photo: %.1f KB", picSize / 1024.0);
      
      delay(2000);  // Show photo for 2 seconds
    }
    
    takeNewPhoto = false;
    isStreaming = true;
  }

  // Main video streaming
  if (isStreaming) {
    streamVideo();
    updateFPS();
    
    // Display FPS every second
    if (millis() - lastFpsTime >= 1000) {
      displayStats();
      lastFpsTime = millis();
    }
  }

  delay(1);  // Small delay to allow other tasks
  
  ArduinoOTA.handle();

}

void checkSPISpeed() {
  Serial.println("Testing SPI speed...");
  
  // Test fillScreen speed
  uint32_t start = millis();
  for(int i = 0; i < 10; i++) {
    LCD.fillScreen(ILI9341_RED);
    LCD.fillScreen(ILI9341_BLUE);
    LCD.fillScreen(ILI9341_GREEN);
  }
  uint32_t end = millis();
  
  float timePerFill = (end - start) / 30.0;  // 30 fill operations
  float pixelsPerSecond = (320.0 * 240.0) / (timePerFill / 1000.0);
  
  Serial.printf("Fill time: %.1f ms per screen\n", timePerFill);
  Serial.printf("Pixel rate: %.1f million pixels/sec\n", pixelsPerSecond / 1000000.0);
  Serial.printf("Estimated SPI speed: %.1f MHz\n", (pixelsPerSecond * 16.0) / 1000000.0); // 16 bits per pixel
}

char IP_Addr[50] = "NULL";

#define WIFI_SWITCH_TIMEOUT_MS 10000
#define WIFI_RECONNECT_DELAY_MS 500

bool initialize_radio_on_wifi() {
  
  // WiFi initialization with timeout
  Serial.println("📶 Initializing WiFi...");
  unsigned long startTime = now_now_ms;
  
  WiFi.disconnect(true, true); // Full cleanup
  delay(WIFI_RECONNECT_DELAY_MS);
  WiFi.mode(WIFI_OFF);
  delay(WIFI_RECONNECT_DELAY_MS);
  
  // Set WiFi mode before initialization
  WiFi.mode(WIFI_STA);
  Serial.printf("📊 WiFi channel: %d\n", WiFi.channel());
  
  // Initialize WiFi with timeout protection
  bool wifiSuccess = false;
 
    while (now_now_ms - startTime < WIFI_SWITCH_TIMEOUT_MS) {
    if (wifi_obj.initialize_ESP_WiFi(devicename) == WIFI_MGR_SUCCESS) {
        wifiSuccess = true;
        break;
    }
    
    Serial.printf("⏳ WiFi connection failed, retrying... (%lu ms elapsed)\n", 
                 now_now_ms - startTime);
    delay(1000);
    
    Serial.printf("⏳ WiFi connection failed, retrying... (%lu ms elapsed)\n", 
                 now_now_ms - startTime);
    delay(1000);
  }
  
  if (!wifiSuccess) {
    Serial.println("💥 Failed to initialize WiFi within timeout period");
    
    // Fallback attempt - try with different approach
    Serial.println("🔄 Attempting fallback WiFi initialization...");
    WiFi.begin(); // Try simple connection
    
    unsigned long fallbackStart = now_now_ms;
    while ((WiFi.status() != WL_CONNECTED) && ((now_now_ms - fallbackStart) < 5000)) {
      delay(500);
      Serial.print(".");
    }
    
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("\n✅ Fallback WiFi connection successful");
      wifiSuccess = true;
    } else {
      Serial.println("\n💥 Fallback WiFi also failed");
      return false;
    }
  }
  
  // Initialize OTA if WiFi is successful
  if (wifiSuccess) {
    Serial.println("🔌 Initializing OTA updates...");
    initializeOTA();
    
    // Verify OTA is ready
    Serial.printf("🎯 WiFi Mode Activated - IP: %s, RSSI: %d dBm\n", 
                 WiFi.localIP().toString().c_str(), WiFi.RSSI());
    return true;
  }
  
  return false;
}


// ----------------------------------------------------------
// Video Streaming Function
// ----------------------------------------------------------
void streamVideo() {
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Camera capture failed");
    delay(50);
    return;
  }

  // Direct JPEG decoding to TFT
  displayFrame(fb);
  
  // Return frame buffer
  esp_camera_fb_return(fb);
  frameCounter++;
}

/*
// ----------------------------------------------------------
// Streaming: capture a single frame and render to TFT
// ----------------------------------------------------------
void streamVideoFrame() {
  // grab frame
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Camera capture failed");
    // small delay to avoid tight loop if camera continually fails
    delay(50);
    return;
  }

  // Make sure it's a JPEG frame (camera configured PIXFORMAT_JPEG)
  if (fb->format != PIXFORMAT_JPEG) {
    Serial.println("Non-JPEG frame received");
    esp_camera_fb_return(fb);
    delay(50);
    return;
  }

  // Optional: update on-screen status once per second
  uint64_t now = esp_timer_get_time();
  frameCount++;
  if ((now - lastFpsTime) >= 1000000) {
    lastFps = frameCount * 1000000.0f / (now - lastFpsTime);
    lastFpsTime = now;
    frameCount = 0;

    // Display FPS and status (bottom bar)
    LCD.fillRect(0, 210, 320, 30, ILI9341_BLACK);
    LCD.setTextColor(ILI9341_GREEN);
    LCD.setCursor(10, 220);
    LCD.setTextSize(2);
    LCD.printf("Streaming: %.1f FPS", lastFps);
  }

  // Decode & render JPEG from buffer directly (no SPIFFS)
  JRESULT jres = TJpgDec.drawJpg(0, 0, fb->buf, fb->len);
  if (jres != JDR_OK) {
    Serial.printf("TJpg decode returned: %d\n", (int)jres);
    // Optionally show error on-screen one time
    // error_log("Decode failed!", ILI9341_YELLOW);
  }

  esp_camera_fb_return(fb);

  // frame pacing (allow other tasks to run, adjust for desired FPS)
  delay(frameDelayMs);
}

*/

// ----------------------------------------------------------
// Display Camera Frame
// ----------------------------------------------------------
void displayFrame(camera_fb_t *fb) {
  if (fb->format != PIXFORMAT_JPEG) {
    Serial.println("Non-JPEG format not supported");
    return;
  }

  // Decode and display JPEG directly from memory
  JRESULT jres = TJpgDec.drawJpg(0, 25, fb->buf, fb->len);
  
  if (jres != JDR_OK) {
    Serial.printf("JPEG decode error: %d\n", jres);
  }
}

// ----------------------------------------------------------
// Buzzer Background Task
// ----------------------------------------------------------
void BuzzingTask(void *pvParams) {
  const TickType_t xDelay = pdMS_TO_TICKS(10);
  while (1) {
    buzzer.update();
    vTaskDelay(xDelay);
  }
}

// ----------------------------------------------------------
// Camera Configuration for Video
// ----------------------------------------------------------
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

  // Optimize for video streaming - faster frame rate
  if (psramFound()) {
    config.frame_size = FRAMESIZE_QVGA;  // FRAMESIZE_QVGA // 320x240 - matches display and faster
    config.jpeg_quality = 12;            // Balanced quality/speed
    config.fb_count = 2;                 // Double buffer for smoother streaming
    config.fb_location = CAMERA_FB_IN_PSRAM;
    config.grab_mode = CAMERA_GRAB_LATEST;  // Always get latest frame
  } else {
    config.frame_size = FRAMESIZE_QVGA;     // 320x240
    config.jpeg_quality = 16;
    config.fb_count = 1;
  }

  esp_err_t rc = esp_camera_init(&config);
  if (rc != ESP_OK) {
    Serial.printf("Camera init failed: 0x%x\n", rc);
    error_log("Camera init failed!", ILI9341_RED);
    delay(2000);
    ESP.restart();
  }

  // Optional: Fine-tune camera settings for video
  sensor_t *s = esp_camera_sensor_get();
  if (s != NULL) {
    //s->set_brightness(s, 0);     // -2 to 2
    //s->set_contrast(s, 0);       // -2 to 2
    //s->set_saturation(s, 0);     // -2 to 2
    s->set_special_effect(s, 0);  // 0 to 6 (0 = no effect)
    s->set_whitebal(s, 1);        // 0 = disable, 1 = enable
    s->set_awb_gain(s, 1);        // 0 = disable, 1 = enable
    s->set_wb_mode(s, 0);         // 0 to 4
    s->set_exposure_ctrl(s, 1);   // 0 = disable, 1 = enable
    s->set_aec2(s, 0);            // 0 = disable, 1 = enable
    s->set_ae_level(s, 0);        // -2 to 2
    s->set_aec_value(s, 300);     // 0 to 1200
    s->set_gain_ctrl(s, 1);       // 0 = disable, 1 = enable
    s->set_agc_gain(s, 0);        // 0 to 30
    s->set_gainceiling(s, (gainceiling_t)0);  // 0 to 6
    s->set_bpc(s, 0);             // 0 = disable, 1 = enable
    s->set_wpc(s, 1);             // 0 = disable, 1 = enable
    s->set_raw_gma(s, 1);         // 0 = disable, 1 = enable
    s->set_lenc(s, 1);            // 0 = disable, 1 = enable
    s->set_hmirror(s, 0);         // 0 = disable, 1 = enable
    s->set_vflip(s, 0);           // 0 = disable, 1 = enable
    s->set_dcw(s, 1);             // 0 = disable, 1 = enable
    s->set_colorbar(s, 0);        // 0 = disable, 1 = enable
  }
}


// ----------------------------------------------------------
// JPEG Decoder Output Callback
// ----------------------------------------------------------
bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t *bitmap) {
  if (x >= LCD.width() || y >= LCD.height()) return false;
  if (x + w > LCD.width()) w = LCD.width() - x;
  if (y + h > LCD.height()) h = LCD.height() - y;

  LCD.startWrite();
  LCD.setAddrWindow(x, y, w, h);
  for (uint32_t i = 0; i < (uint32_t)w * h; i++) {
    LCD.pushColor(bitmap[i]);
    if ((i % 1024) == 0) { 
      yield();  // Prevent watchdog timer issues
    }
  }
  LCD.endWrite();
  return true;
}



// ----------------------------------------------------------
// SPIFFS Helpers (for optional still capture)
// ----------------------------------------------------------
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
  return (s > 10);
}

// ----------------------------------------------------------
// Still Capture and Save (Optional)
// ----------------------------------------------------------
void capturePhotoSaveSpiffs() {
  if (millis() - lastCaptureTime < captureCooldown) return;

  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    successfully_captured = false;
    return;
  }

  File f = SPIFFS.open(FILE_PHOTO, FILE_WRITE);
  if (!f) {
    esp_camera_fb_return(fb);
    successfully_captured = false;
    return;
  }

  f.write(fb->buf, fb->len);
  f.close();
  esp_camera_fb_return(fb);

  successfully_captured = checkPhoto(SPIFFS);
  lastCaptureTime = millis();
}

// ----------------------------------------------------------
// FPS Calculation
// ----------------------------------------------------------
void updateFPS() {
  static uint32_t lastFrameTime = 0;
  static uint32_t frameTimeSum = 0;
  static uint8_t frameCount = 0;
  
  uint32_t currentTime = millis();
  uint32_t frameTime = currentTime - lastFrameTime;
  
  if (lastFrameTime > 0) {
    frameTimeSum += frameTime;
    frameCount++;
    
    if (frameCount >= 10) {  // Average over 10 frames
      fps = 1000.0 / (frameTimeSum / (float)frameCount);
      frameTimeSum = 0;
      frameCount = 0;
    }
  }
  
  lastFrameTime = currentTime;
}

// ----------------------------------------------------------
// Display Statistics
// ----------------------------------------------------------
void displayStats() {
  
  // Show frame counter
  LCD.fillRect(110, 0, 50, 25, ILI9341_BLACK);
  LCD.setCursor(5, 18); LCD.printf("Video Frame:%lu", frameCounter);

  // Show FPS in corner
  LCD.fillRect(225, 0, 30, 25, ILI9341_BLACK);
  LCD.setTextColor(ILI9341_GREEN); LCD.setTextSize(1);
  LCD.setCursor(230, 18);  LCD.printf("%.1f FPS", fps);
  
}

// ----------------------------------------------------------
// On-screen Status Helper
// ----------------------------------------------------------
void error_log(const char *msg, uint16_t color) {
  Serial.println(msg);
  LCD.fillRect(0, 210, 320, 30, ILI9341_BLACK);
  LCD.setTextColor(color);
  LCD.setCursor(10, 220);
  LCD.setTextSize(1);
  LCD.print(msg);
}


void initializeOTA() {
    ArduinoOTA.setHostname(devicename);
    ArduinoOTA.setPassword(OTA_PASS);

    ArduinoOTA
        .onStart([]() { otaStarted = true; }) 
        .onEnd([]() { otaFinished = true;}) 
        .onProgress([](unsigned int progress, unsigned int total) {
           // snprintf(ota_log, sizeof(ota_log), "Progress: %u%%", (progress * 100) / total);
           // flash(now_now_ms, flash, 50, 50, 0, 0);
        })
        .onError([](ota_error_t error) { otaError = true;
            snprintf(ota_log, sizeof(ota_log), "Error[%u]: ", error);
            if (error == OTA_AUTH_ERROR)
                strcat(ota_log, "Auth Failed");
            else if (error == OTA_BEGIN_ERROR)
                strcat(ota_log, "Begin Failed");
            else if (error == OTA_CONNECT_ERROR)
                strcat(ota_log, "Connect Failed");
            else if (error == OTA_RECEIVE_ERROR)
                strcat(ota_log, "Receive Failed");
            else if (error == OTA_END_ERROR)
                strcat(ota_log, "End Failed");
        });

    ArduinoOTA.begin();
}

/*
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
*/
