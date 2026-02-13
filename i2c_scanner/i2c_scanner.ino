#include <Arduino.h>
#include <Wire.h>
//#include "ledc.h"

#define CAM_XCLK 40   // candidate XCLK pin
#define CAM_SIOD 8   // candidate SIOD pin
#define CAM_SIOC 9   // candidate SIOC pin


    const uint8_t channel_1 = 0, channel_2 = 1;
    const uint8_t duty_cycle = 4; // 1 - 16 bits
    const uint16_t pwm_frequency = 12000; // 5kHz up to 30kHz...
    

void startXCLK() {
  
  //ledcSetup(0, 20000000, 1);   // channel 0, 20 MHz, 1-bit duty
  ledcAttachChannel(CAM_XCLK, 20000000, 100, 0);
  ledcWrite(CAM_XCLK, 1);             // 50% duty
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Powering XCLK + scanning I2C...");

  startXCLK();  // feed camera clock

  Wire.begin(CAM_SIOD, CAM_SIOC, 100000);
  delay(200);

  for (uint8_t addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      Serial.printf("Found device at 0x%02X\n", addr);
    }
  }
  Serial.println("Scan done.");
}

void loop() {
  delay(2000);
}
