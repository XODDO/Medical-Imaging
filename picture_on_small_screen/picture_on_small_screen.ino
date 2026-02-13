/*
  Caviscope_CAM_3.ino (Refactored for ILI9225)
  ---------------------------------------------------
  - ESP32-CAM (AI Thinker)
  - 2.0" TFT 176x220 (ILI9225, SPI, no MISO)
  - Displays camera images from SPIFFS (/photo.jpg)
  - OTA + Wi-Fi Manager + Buzzer support
  - Uses TFT_22_ILI9225 library (software SPI)
*/

char * devicename = "Caviscope_CAM_on_Screen";

#include <Arduino.h>
#include <WiFi.h>
#include "esp_camera.h"
#include "esp_timer.h"
#include "img_converters.h"
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "driver/rtc_io.h"
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include <FS.h>
#include <ArduinoOTA.h>

#include "credentials.h"
#include "buzzer.h"
#include "camera_pins.h"
#include "WiFi_Manager.h"

#include <SPI.h>
#include <TFT_22_ILI9225.h>
#include <TJpg_Decoder.h>

// ---------------- TFT Pins (Software SPI, no MISO) ----------------
#define TFT_RST   14
#define TFT_RS    12    // DC
#define TFT_CS    2
#define TFT_SDI   13    // MOSI
#define TFT_CLK   15    // SCK
#define TFT_LED   4     // optional backlight pin

TFT_22_ILI9225 LCD(TFT_CS, TFT_RS, TFT_RST, TFT_SDI, TFT_CLK, TFT_LED);

// ---------------- Globals ----------------
Buzzer buzzer(4);
WiFi_Manager wifi_obj(0);

AsyncWebServer server(80);
const char* FILE_PHOTO = "/photo.jpg";

bool takeNewPhoto = false;
bool wifiConnected = false;
uint8_t err = 0;
bool successfully_captured = false;

char error_logged[120] = ".";
char image_location[40] = "...";
char wifi_network[64] = "No Network";

const unsigned long captureIntervalMs = 2UL * 60UL * 1000UL;
uint64_t lastShot = 0;
unsigned long lastCaptureTime = 0;
const unsigned long captureCooldown = 3000;

// ---------------- Camera Config ----------------
void configure_camera() {
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer   = LEDC_TIMER_0;
  config.pin_d0       = Y2_GPIO_NUM;
  config.pin_d1       = Y3_GPIO_NUM;
  config.pin_d2       = Y4_GPIO_NUM;
  config.pin_d3       = Y5_GPIO_NUM;
  config.pin_d4       = Y6_GPIO_NUM;
  config.pin_d5       = Y7_GPIO_NUM;
  config.pin_d6       = Y8_GPIO_NUM;
  config.pin_d7       = Y9_GPIO_NUM;
  config.pin_xclk     = XCLK_GPIO_NUM;
  config.pin_pclk     = PCLK_GPIO_NUM;
  config.pin_vsync    = VSYNC_GPIO_NUM;
  config.pin_href     = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn     = PWDN_GPIO_NUM;
  config.pin_reset    = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  if (psramFound()) {
    config.frame_size = FRAMESIZE_HD;
    config.jpeg_quality = 12;
    config.fb_count = 1;
    config.fb_location = CAMERA_FB_IN_PSRAM;
  } else {
    config.frame_size = FRAMESIZE_QVGA;
    config.jpeg_quality = 16;
    config.fb_count = 1;
  }

  esp_err_t rc = esp_camera_init(&config);
  if (rc != ESP_OK) {
    Serial.printf("Camera init failed: 0x%x\n", rc);
    err = 13;
    delay(2000);
    ESP.restart();
  }
}

// ---------------- JPEG Decoder Callback ----------------
bool tft_output(int8_t x, int8_t y, int8_t w, int8_t h, unsigned char *bitmap) {
  for (int16_t row = 0; row < h; row++) {
    LCD.drawBitmap(x, y + row, bitmap + row * w, w, 1, COLOR_WHITE);
  }
  return true;
}

// ---------------- SPIFFS Helpers ----------------
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

// ---------------- Capture ----------------
void capturePhotoSaveSpiffs() {
  if (millis() - lastCaptureTime < captureCooldown) return;

  size_t available = SPIFFS.totalBytes() - SPIFFS.usedBytes();
  if (available < 50UL * 1024UL) {
    SPIFFS.remove(FILE_PHOTO);
    delay(10);
  }

  camera_fb_t * fb = esp_camera_fb_get();
  if (!fb) { err = 1; return; }

  File f = SPIFFS.open(FILE_PHOTO, FILE_WRITE);
  if (!f) { err = 2; esp_camera_fb_return(fb); return; }

  size_t written = f.write(fb->buf, fb->len);
  f.close();
  esp_camera_fb_return(fb);

  if (written != fb->len) { err = 19; return; }

  if (checkPhoto(SPIFFS)) {
    successfully_captured = true;
    lastCaptureTime = millis();
  } else {
    successfully_captured = false;
  }
}

// ---------------- Render Image ----------------
void fetchAndRenderImage() {
  LCD.fillRectangle(0, 0, 176, 220, COLOR_BLACK);
  JRESULT jres = TJpgDec.drawJpg(0, 0, FILE_PHOTO);
  if (jres == JDR_OK) {
    Serial.println("Rendered image from SPIFFS");
  } else {
    Serial.printf("TJpg decode returned: %d\n", (int)jres);
  }
}

// ---------------- Error Logger ----------------
void error_log(int log_code) {
  switch (log_code) {
    case 0: strcpy(error_logged, "Camera standby"); break;
    case 1: strcpy(error_logged, "Capture failed"); break;
    case 2: strcpy(error_logged, "File open failed"); break;
    case 4: strcpy(error_logged, "SPIFFS failed"); break;
    case 5: strcpy(error_logged, "Photo Taken OK"); break;
    case 10: strcpy(error_logged, "SPIFFS mount failed"); break;
    case 13: strcpy(error_logged, "Camera init failed"); break;
    case 15: strcpy(error_logged, "Wi-Fi failed"); break;
    default: strcpy(error_logged, "Unknown"); break;
  }

  Serial.println(error_logged);
  LCD.setFont(Terminal6x8);
  LCD.fillRectangle(0, 0, 176, 20, COLOR_BLACK);
  LCD.drawText(5, 5, error_logged, COLOR_YELLOW);
}

// ---------------- OTA ----------------
void initializeOTA() {
  ArduinoOTA.setHostname(devicename);
  ArduinoOTA.begin();
}

// ---------------- Setup ----------------
void setup() {
  Serial.begin(115200);
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

  LCD.begin();
  LCD.setOrientation(3);
  LCD.clear();
  LCD.drawText(10, 10, "Initializing...", COLOR_WHITE);

  ensureSPIFFS();
  configure_camera();

  buzzer.begin();
  xTaskCreatePinnedToCore([](void *){
    while (true) { buzzer.update(); vTaskDelay(pdMS_TO_TICKS(10)); }
  }, "BuzzTask", 2048, NULL, 1, NULL, 1);

  wifi_obj.initialize_ESP_WiFi(devicename);
  wifiConnected = (WiFi.status() == WL_CONNECTED);
  initializeOTA();

  TJpgDec.setCallback(tft_output);
  TJpgDec.setJpgScale(1);

  LCD.clear();
  LCD.drawText(10, 10, "Ready", COLOR_CYAN);
}

// ---------------- Loop ----------------
void loop() {
  if ((esp_timer_get_time() - lastShot) >= (int64_t)captureIntervalMs * 1000) {
    takeNewPhoto = true;
    lastShot = esp_timer_get_time();
  }

  if (takeNewPhoto) {
    error_log(17);
    capturePhotoSaveSpiffs();
    if (successfully_captured) fetchAndRenderImage();
    takeNewPhoto = false;
  }

  ArduinoOTA.handle();
  delay(1);
}
