#include <Wire.h>

#define CAM_SIOD 8   // try candidate SIOD pin
#define CAM_SIOC 9   // try candidate SIOC pin

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Camera I2C probe starting...");

  // Initialize I2C with candidate pins
  Wire.begin(CAM_SIOD, CAM_SIOC, 100000);  // SDA, SCL, 100kHz
  delay(200);

  Serial.println("Scanning I2C bus...");
  for (uint8_t addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      Serial.printf("Found device at 0x%02X\n", addr);
      delay(5);
    }
  }
  Serial.println("Scan done.");
}

void loop() {
  delay(2000);
}
