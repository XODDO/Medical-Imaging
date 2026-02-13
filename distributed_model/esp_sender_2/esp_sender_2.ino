#include "esp_camera.h"
#include "FS.h"
#include "SD_MMC.h"
#include <Arduino.h>

// ==== CAMERA PINS (AI Thinker ESP32-CAM) ====
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

#define FILE_PHOTO "/photo.jpg"

// ==== UART0 PINS ====
#define UART0_TX 1   // U0TXD
#define UART0_RX 3   // U0RXD
#define UART_BAUD 921600

void setup() {
  // Boot debug on Serial
  Serial.begin(115200);
  delay(2000);
  Serial.println("\nBooting ESP32-CAM Sender...");

  // Initialize SD card
  if(!SD_MMC.begin()){
    Serial.println("SD Card Mount Failed!");
    while(1);
  }
  Serial.println("SD Card initialized.");

  // ==== Camera Config ====
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
  config.frame_size = FRAMESIZE_QVGA; 
  config.jpeg_quality = 12;
  config.fb_count = 1;

  if(esp_camera_init(&config) != ESP_OK){
    Serial.println("Camera init failed!");
    while(1);
  }
  Serial.println("Camera ready.");

  // Initialize UART0 (pins 1 & 3 are fixed, no need to remap)
  Serial1.begin(UART_BAUD, SERIAL_8N1, UART0_RX, UART0_TX);
  Serial.println("UART0 ready for transfer.");
}

void loop() {
  // ==== Capture Photo ====
  camera_fb_t * fb = esp_camera_fb_get();
  if(!fb){
    Serial.println("Camera capture failed!");
    delay(2000);
    return;
  }

  // Save to SD
  File file = SD_MMC.open(FILE_PHOTO, FILE_WRITE);
  if(file){
    file.write(fb->buf, fb->len);
    file.close();
    Serial.printf("Photo saved (%d bytes).\n", fb->len);
  } else {
    Serial.println("SD write failed!");
  }

  // Release camera buffer
  esp_camera_fb_return(fb);

  // ==== Send file via UART0 ====
  file = SD_MMC.open(FILE_PHOTO, FILE_READ);
  if(file){
    size_t fileSize = file.size();
    Serial.printf("Sending photo (%u bytes) via UART0...\n", fileSize);

    // Send header
    Serial1.print("PHOTO");
    Serial1.write((uint8_t*)&fileSize, sizeof(fileSize));

    // Send file content
    uint8_t buf[512];
    size_t sent = 0;
    while(file.available()){
      size_t len = file.read(buf, sizeof(buf));
      Serial1.write(buf, len);
      sent += len;
    }
    file.close();

    // Send footer
    Serial1.print("END");
    Serial.printf("Photo sent! (%u bytes)\n", sent);
  }

  delay(5000); // wait 5s before next photo
}
