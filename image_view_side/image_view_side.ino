#include <WiFi.h>
#include <HTTPClient.h>
#include <TJpg_Decoder.h>
#include <TFT_eSPI.h>

const char* ssid = "irrikit_Cloud";
const char* password = "IntelliSys@2025!";
const char* camIP = "http://192.168.1.2/saved-photo"; // Replace with your ESP32-CAM endpoint

TFT_eSPI tft = TFT_eSPI();  // For MakerGo 7" or ILI9341

// ==== JPEG draw callback ====
bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap) {
  if (y >= tft.height()) return 0;
  tft.pushImage(x, y, w, h, bitmap);
  return 1;
}

// ==== Setup ====
void setup() {
  Serial.begin(115200); delay(100); Serial.println("Booting...");
  Serial.println("Connecting WiFi...");
  
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");
  Serial.println(WiFi.localIP());

  tft.begin();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);

  TJpgDec.setCallback(tft_output);
//TJpgDec.setJpgScale(1);   // full resolution
  TJpgDec.setJpgScale(4);  //  show at quarter 1280×720 => 320x180 on a 320x240 DISP

  fetchAndRenderImage();
}

// ==== Fetch and decode JPEG ====
void fetchAndRenderImage() {

  HTTPClient http;
  http.begin(camIP);
  int httpCode = http.GET();

  if (httpCode == 200) {
    int len = http.getSize();
    WiFiClient* stream = http.getStreamPtr();

    // allocate buffer in PSRAM if available
    uint8_t *jpgBuf = (uint8_t *)ps_malloc(len);
    if (!jpgBuf) {
      Serial.println("Memory allocation failed!");
      http.end();
      return;
    }

    int bytesRead = 0;
    unsigned long start = millis();
    while (http.connected() && (bytesRead < len)) {
      int available = stream->available();
      if (available) {
        int bytes = stream->readBytes(jpgBuf + bytesRead, available);
        bytesRead += bytes;
      } else {
        delay(1);
      }
    }
    Serial.printf("Downloaded %d bytes in %lu ms\n", bytesRead, millis() - start);

    // Decode JPEG from memory buffer
   // TJpgDec.usePsram(true);

    Serial.printf("Free heap: %d bytes\n", ESP.getFreeHeap()); //If is < 80 000, decoding large images will fail.

    TJpgDec.drawJpg(0, 0, jpgBuf, bytesRead);
    //TJpgDec.drawJpg(0, 0, stream); // decoding directly from the stream

    free(jpgBuf);
  } else {
    Serial.printf("HTTP error: %d\n", httpCode);
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.drawString("Fetch failed!", 10, 10, 4);
  }

  http.end();
}

void loop() {
  // Periodically refresh
  delay(10000);
  fetchAndRenderImage();
}
