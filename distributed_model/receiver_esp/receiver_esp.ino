#include <WiFi.h>
#include <HTTPClient.h>
#include "FS.h"
#include "SPIFFS.h"

#define SERIAL2_RX 16
#define SERIAL2_TX 17
#define SERIAL2_BAUD 921600

// WiFi credentials
const char* ssid = "IntelliSys Cloud";
const char* password = "IntelliSys@2025!";

// Server URL to send image
const char* serverURL = "https://www.webofiot.com/caviscope/upload";

void setup() {
  Serial.begin(921600);
  Serial2.begin(SERIAL2_BAUD, SERIAL_8N1, SERIAL2_RX, SERIAL2_TX);

  if(!SPIFFS.begin(true)){
    Serial.println("SPIFFS Mount Failed");
    while(1);
  }

  WiFi.begin(ssid, password);
  while(WiFi.status() != WL_CONNECTED){
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected: " + WiFi.localIP().toString());

  Serial.println("Waiting for Image...")
}

void loop() {
  // Look for PHOTO header
  if(Serial2.find("PHOTO")){
    size_t fileSize;
    Serial2.readBytes((char*)&fileSize, sizeof(fileSize));
    Serial.printf("Incoming photo: %u bytes\n", fileSize);

    File file = SPIFFS.open("/rx_photo.jpg", FILE_WRITE);
    size_t received = 0;
    uint8_t buf[512];

    while(received < fileSize){
      size_t toRead = min(sizeof(buf), fileSize - received);
      size_t len = Serial2.readBytes(buf, toRead);
      if(len > 0){
        file.write(buf, len);
        received += len;
      }
    }
    file.close();

    // Optional: wait for END marker
    Serial2.find("END");
    Serial.println("Photo received in SPIFFS");

    // Send via HTTP POST
    if(WiFi.status() == WL_CONNECTED){
      HTTPClient http;
      http.begin(serverURL);
      http.addHeader("Content-Type", "image/jpeg");

      File f = SPIFFS.open("/rx_photo.jpg", "r");
      int code = http.sendRequest("POST", &f, f.size());
      f.close();

      if(code > 0) Serial.printf("Upload OK, code %d\n", code);
      else Serial.printf("Upload failed: %s\n", http.errorToString(code).c_str());

      http.end();
    }
  }
}
