#include "esp_camera.h"
#include "FS.h"
#include "SD_MMC.h"
#include <Arduino.h>

// Camera pins for AI-Thinker ESP32-CAM
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

// Serial2 pins
#define SERIAL2_RX 13
#define SERIAL2_TX 15
#define SERIAL2_BAUD 1000000
//#define SERIAL2_BAUD 921600

/*
The Best Trick: Use Serial2.begin(baud, config, RXpin, TXpin) with any free GPIO

The ESP32’s UARTs are flexible thanks to the IO MUX — you can remap RX/TX to almost any GPIO.

Example:

Serial2.begin(921600, SERIAL_8N1, 14, 15);  // RX=GPIO14, TX=GPIO15


Then you just wire those free GPIOs to your second ESP32.

On the ESP32-CAM, usable candidates include GPIO 12, 13, 14, 15 
(SD pins, but available if you don’t use SD), or even GPIO2 (LED flash pin) if you’re careful.
Recommendation

If you need SD card active:

You’re tight on pins. In that case, reuse U0RXD/U0TXD (GPIO3, GPIO1) after boot.

If you don’t need SD card:

Re-map UART2 to GPIO12/13/14/15 for TX/RX.
*/

void setup() {
  //Serial.begin(115200);
  Serial.begin(SERIAL2_BAUD, SERIAL_8N1, 3, 1);
  Serial2.begin(SERIAL2_BAUD, SERIAL_8N1, SERIAL2_RX, SERIAL2_TX);

  // Initialize SD card
  if(!SD_MMC.begin()){
    Serial.println("SD Card Mount Failed");
    while(1);
  }

  // Camera config
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
    Serial.println("Camera Init Failed");
    while(1);
  }

  Serial.println("ESP32-CAM ready");
}

void loop() {
  // Capture photo
  camera_fb_t * fb = esp_camera_fb_get();
  if(!fb){
    Serial.println("Camera capture failed");
    delay(1000);
    return;
  }

  // Save to SD
  File file = SD_MMC.open(FILE_PHOTO, FILE_WRITE);
  if(file){
    file.write(fb->buf, fb->len);
    file.close();
    Serial.println("Photo saved to SD");
  }
  esp_camera_fb_return(fb);

  // Send file via Serial2
  file = SD_MMC.open(FILE_PHOTO, FILE_READ);
  if(file){
    size_t fileSize = file.size();

    // Header
    Serial.print("PHOTO");
    Serial.write((uint8_t*)&fileSize, sizeof(fileSize));

    // File data
    uint8_t buf[512];
    while(file.available()){
      size_t len = file.read(buf, sizeof(buf));
      Serial.write(buf, len);
    }
    file.close();

    // Footer
    Serial.print("END");
    //Serial.println("Photo sent over Serial2");
  }

  delay(5000); // wait 5s before next capture
}
