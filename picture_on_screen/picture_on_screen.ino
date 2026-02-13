/*
  Caviscope_CAM_ILI9341.ino
  ---------------------------------------------------
  - ESP32-CAM (AI Thinker)
  - 2.8" TFT (ILI9341, hardware SPI)
  - Auto-captures image once every minute
  - Saves to SPIFFS (/photo.jpg)
  - Displays photo immediately
  - Includes buzzer + on-screen feedback
*/

char *devicename = "Caviscope_CAM_ILI9341";

#include <Arduino.h>
#include "esp_camera.h"
#include "esp_timer.h"
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "driver/rtc_io.h"
#include <SPIFFS.h>
#include <FS.h>
#include <TJpg_Decoder.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include "buzzer.h"
#include "camera_pins.h"

// ---------- TFT Pins ----------
#define TFT_CS   2
#define TFT_DC   12
#define TFT_RST  14
#define TFT_MOSI 13
#define TFT_CLK  15
#define TFT_LED  4   // optional backlight

// ---------- TFT + Buzzer ----------
Adafruit_ILI9341 LCD = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_MOSI, TFT_CLK, TFT_RST);
Buzzer buzzer(TFT_LED);  // reuse LED pin if no dedicated buzzer pin

// ---------- Globals ----------
const char *FILE_PHOTO = "/photo.jpg";
bool takeNewPhoto = false;
bool successfully_captured = false;

const unsigned long captureIntervalMs = 1* 60UL * 1000UL;  // 2X / minute
unsigned long lastCaptureTime = 0;
const unsigned long captureCooldown = 3000;

uint64_t lastShot = 0;

// ---------- Forward Declarations ----------
void BuzzingTask(void *pvParams);
void configure_camera();
bool ensureSPIFFS();
bool checkPhoto(fs::FS &fs);
void capturePhotoSaveSpiffs();
void fetchAndRenderImage();
bool tft_output(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t *bitmap);
void error_log(const char *msg, uint16_t color = ILI9341_YELLOW);

// ----------------------------------------------------------
// Setup
// ----------------------------------------------------------
void setup() {
  Serial.begin(115200);
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

  // TFT Init
  LCD.begin();
  LCD.setRotation(3);
  LCD.fillScreen(ILI9341_BLACK);
  LCD.setTextColor(ILI9341_CYAN);
  LCD.setTextSize(2);
  LCD.setCursor(10, 20);
  LCD.println("Caviscope_CAM_ILI9341");
  LCD.setTextSize(1);
  LCD.println("Initializing...");

  // Optional: backlight ON
  pinMode(TFT_LED, OUTPUT);
  digitalWrite(TFT_LED, HIGH);

  // SPIFFS
  if (!ensureSPIFFS()) {
    error_log("SPIFFS mount failed!", ILI9341_RED);
    delay(2000);
    ESP.restart();
  }

  // Camera
  configure_camera();

  // Buzzer
  buzzer.begin();
  xTaskCreatePinnedToCore(BuzzingTask, "BuzzingCheck", 2048, NULL, 1, NULL, 1);

  // JPEG Decoder
  TJpgDec.setCallback(tft_output);
  TJpgDec.setJpgScale(2); // half the size to fit a High Quality image onto the 320x240px disp

  LCD.fillScreen(ILI9341_BLACK);
  error_log("Ready.", ILI9341_GREEN);
}

// ----------------------------------------------------------
// Loop
// ----------------------------------------------------------
void loop() {
  if ((esp_timer_get_time() - lastShot) >= (int64_t)captureIntervalMs * 1000) {
    takeNewPhoto = true;
    lastShot = esp_timer_get_time();
  }

  if (takeNewPhoto) {
    error_log("Capturing...", ILI9341_YELLOW);
    capturePhotoSaveSpiffs();

    if (successfully_captured) {
      buzzer.beep(1, 50, 0);
      
      fetchAndRenderImage();
      


      // --- Display file size ---
      File f = SPIFFS.open(FILE_PHOTO, "r");
      size_t picSize = f.size();
      f.close();

      //error_log("Photo Taken [kB]", ILI9341_GREEN);

      LCD.fillRect(0, 210, 320, 30, ILI9341_BLACK);
      LCD.setTextColor(ILI9341_GREEN);
      LCD.setCursor(10, 220);
      LCD.setTextSize(2);
      LCD.printf("Photo: %.1f KB", picSize / 1024.0);

      Serial.printf("Photo size: %.1f KB\n", picSize / 1024.0);

    } else {
      error_log("Capture failed!", ILI9341_RED);
    }

    takeNewPhoto = false;
  }

  delay(10);
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
// Camera Configuration
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

if (psramFound()) {
  config.frame_size = FRAMESIZE_VGA;  // FRAMESIZE_VGA [640x480] instead of SVGA [800×600]
  config.jpeg_quality = 10;
  config.fb_count = 2;
  config.fb_location = CAMERA_FB_IN_PSRAM;
} else {
  config.frame_size = FRAMESIZE_QVGA; // 320x240
  config.jpeg_quality = 16;
  config.fb_count = 1;
}

/*
  if (psramFound()) {
    config.frame_size = FRAMESIZE_SVGA;  // 800x600
    config.jpeg_quality = 12;
    config.fb_count = 2;
    config.fb_location = CAMERA_FB_IN_PSRAM;
    
  } else {
    config.frame_size = FRAMESIZE_VGA;   // 640x480
    config.jpeg_quality = 16;
    config.fb_count = 1;
  }
*/
  esp_err_t rc = esp_camera_init(&config);
  if (rc != ESP_OK) {
    Serial.printf("Camera init failed: 0x%x\n", rc);
    error_log("Camera init failed!", ILI9341_RED);
    delay(2000);
    ESP.restart();
  }
}
/*
// ----------------------------------------------------------
// JPEG Decoder Output Callback
// ----------------------------------------------------------
// ---------------- JPEG Decoder Callback ----------------
// This matches TJpg_Decoder's required signature
bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t *bitmap) {
  // Adafruit_ILI9341 expects 16-bit color pixels (RGB565)
  LCD.startWrite();
  LCD.setAddrWindow(x, y, w, h);

  // Library version 1.6.2 has no pushColors(), so push pixels one by one
  for (uint32_t i = 0; i < (uint32_t)w * h; i++) {
    LCD.pushColor(bitmap[i]);
  }

  LCD.endWrite();
  return true;
}

*/
bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t *bitmap) {
  if (x >= LCD.width() || y >= LCD.height()) return false;
  if (x + w > LCD.width()) w = LCD.width() - x;
  if (y + h > LCD.height()) h = LCD.height() - y;

  LCD.startWrite();
  LCD.setAddrWindow(x, y, w, h);
  for (uint32_t i = 0; i < (uint32_t)w * h; i++) {
    LCD.pushColor(bitmap[i]);
    if ((i % 1024) == 0) { yield(); /*Serial.println("Yielded!");*/}

  }
  LCD.endWrite();
  return true;
}



// ----------------------------------------------------------
// SPIFFS Helpers
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
// Capture and Save
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
// Display Captured Image
// ----------------------------------------------------------
void fetchAndRenderImage() {

  File f = SPIFFS.open(FILE_PHOTO, "r");
  Serial.printf("JPEG size: %u bytes\n", f.size());
  f.close();


  LCD.fillScreen(ILI9341_BLACK);
  JRESULT jres = TJpgDec.drawJpg(0, 0, FILE_PHOTO);
  if (jres == JDR_OK) {
    Serial.println("Rendered image from SPIFFS");
  } else {
    Serial.printf("TJpg decode returned: %d\n", (int)jres);
  }
}

// ----------------------------------------------------------
// On-screen Status Helper
// ----------------------------------------------------------
void error_log(const char *msg, uint16_t color) {
  Serial.println(msg);
  LCD.fillRect(0, 210, 320, 30, ILI9341_BLACK);
  LCD.setTextColor(color);
  LCD.setCursor(10, 220);
  LCD.setTextSize(2);
  LCD.print(msg);
}
