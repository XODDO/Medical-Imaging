char * devicename = "Photo_VIEWER";
#include <WiFi.h>
#include <HTTPClient.h>
#include <TJpg_Decoder.h>
#include <TFT_eSPI.h>
#include "WiFi_Manager.h"

const char* camIP = "http://192.168.0.101/saved-photo"; // Replace with your ESP32-CAM endpoint

TFT_eSPI tft = TFT_eSPI();

/*

    // ===== CALLBACK =====
    bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t *bitmap) {
      if (y >= tft.height()) return 0;  // safety check
      tft.pushImage(x, y, w, h, bitmap);
      return 1;  // continue decoding
    }

*/

bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap){
    for (int i = 0; i < w * h; i++) {
        uint16_t color = bitmap[i];
        bitmap[i] = (color >> 8) | (color << 8); // swap bytes
    }
    tft.pushImage(x, y, w, h, bitmap);
    return true;
}


uint8_t wifi_led = 27;

WiFi_Manager wifi_obj(wifi_led);  // LED on GPIO2


bool wifi_connected = false;

uint8_t status_code = 0;

void setup() {
  Serial.begin(115200); delay(500);
  Serial.println("\nCAVISCOPE DISPLAY UNIT");

  // --- Display setup ---
  tft.begin();
  tft.setRotation(1); // landscape
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString("Initializing WiFi..", 10, 2);

   // --- Wi-Fi setup ---
    Serial.println("Init WiFi...");
    status_code = wifi_obj.initialize_ESP_WiFi(devicename);
    // log_status(status_code);


  // --- TJpg_Decoder setup ---
  TJpgDec.setCallback(tft_output);
  TJpgDec.setJpgScale(1);   // reduce resolution by half for stability
  Serial.println("TJpg_Decoder ready.");

  delay(1000);
  fetchAndRenderImage();

   
}

 unsigned long long last_screen_refresh = 0;
  unsigned long long last_wifi_check = 0;
void loop() {
  // periodic refresh every 10 seconds for testing
  if (esp_timer_get_time() - last_screen_refresh > 10000000ULL) { // every 10 seconds
    fetchAndRenderImage();
    last_screen_refresh = esp_timer_get_time();
  }

  if((esp_timer_get_time() - last_wifi_check) > (5*1000000ULL)){ // EVERY 5 SECONDS
       wifi_connected = wifi_obj.ensure_wifi();  // a task that checks it once every 5 seconds
       last_wifi_check = esp_timer_get_time();
       if(!wifi_connected) { status_code = 10; Serial.println("Cannot Upload without WiFi!!!"); return; }
  }


          
}



char picture_log[100] = "...";

void fetchAndRenderImage() {
  HTTPClient http;
  http.begin(camIP);
  int code = http.GET();

  if (code == HTTP_CODE_OK) {
    int len = http.getSize();
    WiFiClient *stream = http.getStreamPtr();

    // Allocate memory for the image
    uint8_t *jpgBuf = (uint8_t *)malloc(len);
    if (!jpgBuf) {
      Serial.println("Memory allocation failed!");
      http.end();
      return;
    }

    // Read the image into RAM
    int index = 0;
    while (http.connected() && (len > 0 || len == -1)) {
      size_t available = stream->available();
      if (available) {
        int bytesRead = stream->readBytes(jpgBuf + index, available);
        index += bytesRead;
        if (len > 0) len -= bytesRead;
      }
      delay(1);
    }
        char bayts[10] = ".";
        itoa(index, bayts, 10);
        strcpy(picture_log, "Picture successfully downloaded. | Size: ");
        strcat(picture_log, bayts);
        strcat(picture_log, "B");
        
    Serial.println(picture_log);
    tft.setCursor(10, 25); tft.print(picture_log);

    // Decode and display the image
    TJpgDec.setJpgScale(1); // Adjust to 2 or 4 if low on RAM
    TJpgDec.setCallback(tft_output);
    TJpgDec.drawJpg(0, 40, jpgBuf, index);

    free(jpgBuf); // Release RAM
  } else {
    Serial.printf("HTTP error: %d\n", code);
  }

  http.end();
}

