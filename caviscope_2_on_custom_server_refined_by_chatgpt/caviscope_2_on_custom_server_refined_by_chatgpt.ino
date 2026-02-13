/*
Perfect 👍 — let’s harden your ESP32-CAM sketch.
I’ll keep the same functionality (capture → save → serve photo) but add error handling, safe retries, and reduce flash wear.

Key fixes:

    ✅ Add WiFi connect timeout + fallback AP.

    ✅ Limit photo save retries (MAX_RETRIES).

    ✅ Check file validity before .size().

    ✅ Use timestamped filenames (optional) or overwrite only if necessary → reduces repeated wear on same SPIFFS block.

    ✅ Smaller default resolution unless UXGA is really needed.

    ✅ Fail gracefully instead of infinite loops.

🔎 Why you see DMA overflow

Too high frame size (resolution)

FRAMESIZE_UXGA (1600x1200) or even XGA can overwhelm the ESP32’s bandwidth, especially if PSRAM is slow.

The camera is delivering pixels faster than the DMA engine can transfer them to memory.

JPEG quality too high (low compression)

Lower jpeg_quality (e.g. 10) = larger image sizes = more DMA pressure.

No PSRAM or poor PSRAM

ESP32-CAM relies on external PSRAM to buffer large frames. Some clones have flaky PSRAM chips → buffer overruns.

fb_count too small

With only 1 framebuffer, DMA has nowhere to put new data while the CPU is processing the previous frame.

Clock speed mismatch

config.xclk_freq_hz = 20000000; is correct, but in some boards, lowering to 10 MHz helps reduce overflow.

Here’s the rewritten core sketch:
*/

#include "WiFi.h"
#include "esp_camera.h"
#include "esp_timer.h"
#include "img_converters.h"
#include "Arduino.h"
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "driver/rtc_io.h"
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include <FS.h>

// ================== CONFIG ==================
const char* ssid     = "IntelliSys Online";
const char* password = "qwerty0976_";

#define FILE_PHOTO "/photo.jpg"
#define MAX_RETRIES 3          // limit retries to avoid infinite loop
#define WIFI_TIMEOUT_MS 20000  // WiFi timeout 20s

AsyncWebServer server(80);
bool takeNewPhoto = false;

// Camera pin mapping (AI Thinker)
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

// ================== HTML ==================
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style> body{text-align:center;} </style>
</head>
<body>
  <h2>ESP32-CAM Last Photo</h2>
  <p><button onclick="capturePhoto()">CAPTURE PHOTO</button>
     <button onclick="location.reload();">REFRESH</button></p>
  <div><img src="saved-photo" id="photo" width="70%"></div>
<script>
  function capturePhoto(){ var x=new XMLHttpRequest(); x.open('GET',"/capture",true); x.send();}
</script>
</body></html>)rawliteral";

// ================== WIFI ==================
void connectWiFi() {
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  unsigned long startAttempt = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startAttempt < WIFI_TIMEOUT_MS) {
    delay(500);
    Serial.print(".");
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("\nWiFi connected, IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nWiFi connect failed. Starting fallback AP...");
    WiFi.softAP("ESP32-CAM_AP");
    Serial.print("AP IP address: ");
    Serial.println(WiFi.softAPIP());
  }
}

// ================== PHOTO HANDLING ==================
bool checkPhoto(fs::FS &fs) {
  if (!fs.exists(FILE_PHOTO)) return false;
  File f_pic = fs.open(FILE_PHOTO);
  if (!f_pic || f_pic.isDirectory()) return false;
  size_t pic_sz = f_pic.size();
  f_pic.close();
  return (pic_sz > 100); // sanity check
}

bool capturePhotoSaveSpiffs() {
  camera_fb_t * fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Camera capture failed");
    return false;
  }

  File file = SPIFFS.open(FILE_PHOTO, FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open file in writing mode");
    esp_camera_fb_return(fb);
    return false;
  }

  size_t written = file.write(fb->buf, fb->len);
  file.close();
  esp_camera_fb_return(fb);

  if (written != fb->len) {
    Serial.println("File write incomplete, capture invalid.");
    return false;
  }

  Serial.printf("Photo saved: %s (%d bytes)\n", FILE_PHOTO, written);
  return checkPhoto(SPIFFS);
}

// ================== SETUP ==================
void setup() {
  Serial.begin(115200);

  // Use stable power supply instead of disabling brownout
  // WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);  // keep brownout ON for safety

  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS Mount Failed, restarting...");
    ESP.restart();
  }

  connectWiFi();

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
  config.xclk_freq_hz = 10000000;
  config.pixel_format = PIXFORMAT_JPEG;

  if (psramFound()) {
      config.frame_size = FRAMESIZE_VGA;   // safer than UXGA
      config.jpeg_quality = 25; // Increase jpeg_quality (more compression)
                             // e.g., from 10 → 12–15. Smaller output = fewer DMA overflows.
    config.fb_count = 2;
  } else {
      config.frame_size = FRAMESIZE_VGA;
      config.jpeg_quality = 15;
      config.fb_count = 1;
  }

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed: 0x%x", err);
    ESP.restart();
  }

  // Routes
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/html", index_html);
  });

  server.on("/capture", HTTP_GET, [](AsyncWebServerRequest *request) {
    takeNewPhoto = true;
    request->send(200, "text/plain", "Taking Photo...");
  });

  server.on("/saved-photo", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (SPIFFS.exists(FILE_PHOTO)) {
      request->send(SPIFFS, FILE_PHOTO, "image/jpg");
    } else {
      request->send(404, "text/plain", "No photo saved");
    }
  });

  server.begin();
}

// ================== LOOP ==================
void loop() {
  if (takeNewPhoto) {
    bool success = false;
    for (int i = 0; i < MAX_RETRIES && !success; i++) {
      Serial.printf("Attempt %d to capture photo...\n", i + 1);
      success = capturePhotoSaveSpiffs();
    }
    if (!success) {
      Serial.println("ERROR: Failed to save photo after retries.");
    }
    takeNewPhoto = false;
  }
  delay(1);
}
