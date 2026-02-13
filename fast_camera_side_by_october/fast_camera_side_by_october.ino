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
#include "credentials.h"    // your list of known Wi-Fi networks

// ====== Wi-Fi Handling ======
unsigned long lastWiFiAttempt = 0;
const unsigned long reconnectInterval = 15000; // ms
bool wifiConnected = false;

AsyncWebServer server(80);
boolean takeNewPhoto = false;
#define FILE_PHOTO "/photo.jpg"

// ====== CAMERA PINS (AI Thinker) ======
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

// ====== WEB PAGE ======
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1">
<style> body { text-align:center; } </style>
</head>
<body>
  <h2>ESP32-CAM Laparoscope Capture</h2>
  <p><button onclick="capturePhoto()">CAPTURE</button></p>
  <img src="saved-photo" id="photo" width="70%">
  <script>
    function capturePhoto() {
      fetch('/capture').then(()=>setTimeout(()=>location.reload(),3000));
    }
  </script>
</body>
</html>)rawliteral";

uint8_t err = 0;
char error_logged[100] = ".";

// ====== Function prototypes ======
void capturePhotoSaveSpiffs(void);
bool checkPhoto(fs::FS &fs);
void error_log(int log_count);
bool connectKnownWiFi(void);

// ====== SETUP ======
void setup() {
  //Serial.begin(115200);
  Serial.begin(1000000); delay(500);
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); // disable brownout detection

  if (!SPIFFS.begin(true)) {
    err = 10;  // SPIFFS mount failure
    error_log(err);
    delay(1000);
    ESP.restart();
  }

  err = 11;  // Starting WiFi scan
  error_log(err);
  if (!connectKnownWiFi()) {
    err = 12;  // No network found
    error_log(err);
    delay(2000);
    ESP.restart();
  }

  configure_camera();

  // ====== WEB ROUTES ======
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", index_html);
  });
  server.on("/capture", HTTP_GET, [](AsyncWebServerRequest *request) {
    takeNewPhoto = true;
    request->send(200, "text/plain", "Capturing photo...");
  });
  server.on("/saved-photo", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, FILE_PHOTO, "image/jpg", false);
  });

  server.begin();
  err = 14;  // server started
  error_log(err);
}

  // ====== CAMERA CONFIG ======
void configure_camera(){
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
    Serial.println("PSRAM detected ✅ — using SVGA.");
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 2;
  } else {
    Serial.println("No PSRAM ⚠️ — using smaller frame.");
    config.frame_size = FRAMESIZE_QVGA;
    config.jpeg_quality = 20;
    config.fb_count = 1;
  }

  esp_err_t err_cam = esp_camera_init(&config);
  if (err_cam != ESP_OK) {
    err = 13;  // camera init failed
    error_log(err);
    delay(2000);
    ESP.restart();
  }
}

// ====== MAIN LOOP ======
void loop() {
  if (takeNewPhoto) {
    capturePhotoSaveSpiffs();
    error_log(err);
    takeNewPhoto = false;
  } else err = 0;

  // Non-blocking Wi-Fi reconnection logic
  if (WiFi.status() != WL_CONNECTED && millis() - lastWiFiAttempt > reconnectInterval) {
    Serial.println("Reconnecting to any of the known Wi-Fi networks...");
    err = 16;  // Wi-Fi lost, retrying
    error_log(err);
    wifiConnected = connectKnownWiFi();
    lastWiFiAttempt = millis();
  }

  delay(5);
}

// ====== Wi-Fi Connect Cycle ======
bool connectKnownWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(true);
  delay(100);

  err = 11;  // scanning
  error_log(err);
  int n = WiFi.scanNetworks();
  if (n <= 0) return false;

  // Track best known SSID by RSSI
  int bestIndex = -1;
  int bestRSSI = -999;

  for (int i = 0; i < n; i++) {
    String foundSSID = WiFi.SSID(i);
    int32_t rssi = WiFi.RSSI(i);
    for (int j = 0; j < knownCount; j++) {
      if (foundSSID == knownNetworks[j].SECRET_SSID) {
        Serial.printf("Found known SSID: %s (%ddBm)\n", foundSSID.c_str(), rssi);
        if (rssi > bestRSSI) {
          bestRSSI = rssi;
          bestIndex = j;
        }
      }
    }
  }

  if (bestIndex == -1) {
    err = 12;  // no known Wi-Fi found
    error_log(err);
    return false;
  }

  // Try connecting to the strongest known SSID
  Serial.printf("Connecting to strongest known network: %s (%ddBm)\n",
                knownNetworks[bestIndex].SECRET_SSID, bestRSSI);
  WiFi.begin(knownNetworks[bestIndex].SECRET_SSID, knownNetworks[bestIndex].SECRET_PASS);

  unsigned long startAttempt = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startAttempt < 10000) {
    delay(100);
    yield();
  }

  if (WiFi.status() == WL_CONNECTED) {
    err = 15;  // Wi-Fi connected
    error_log(err);
    Serial.printf("✅ Connected to %s | IP: %s\n",
                  WiFi.SSID().c_str(),
                  WiFi.localIP().toString().c_str());
    wifiConnected = true;
    return true;
  } else {
    err = 16;  // connection attempt failed
    error_log(err);
    wifiConnected = false;
    return false;
  }
}

// ====== CAPTURE PHOTO ======
bool checkPhoto(fs::FS &fs) {
  File f_pic = fs.open(FILE_PHOTO);
  if (!f_pic || f_pic.size() < 100) {
    f_pic.close();
    return false;
  }
  f_pic.close();
  return true;
}

void capturePhotoSaveSpiffs(void) {
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) { err = 1; return; }

  File file = SPIFFS.open(FILE_PHOTO, FILE_WRITE);
  if (!file) {
    err = 2;
    esp_camera_fb_return(fb);
    return;
  }

  file.write(fb->buf, fb->len);
  file.close();
  esp_camera_fb_return(fb);
  fb = NULL;
  err = checkPhoto(SPIFFS) ? 5 : 4;
}

// ====== ERROR LOGGER ======
void error_log(int log_count) {
  switch (log_count) {
    case 0: strcpy(error_logged, "Camera on standby..."); break;
    case 1: strcpy(error_logged, "Camera capture failed ❌"); break;
    case 2: strcpy(error_logged, "File open failed ❌"); break;
    case 4: strcpy(error_logged, "Photo verification failed ❌"); break;
    case 5: {
      File f = SPIFFS.open(FILE_PHOTO);
      if (f) {
        snprintf(error_logged, sizeof(error_logged),
                 "Photo saved ✅: %s (%d bytes)", FILE_PHOTO, f.size());
        f.close();
      } else strcpy(error_logged, "Photo saved ✅");
      break;
    }
    case 10: strcpy(error_logged, "SPIFFS mount failed ❌"); break;
    case 11: strcpy(error_logged, "Scanning for Wi-Fi networks..."); break;
    case 12: strcpy(error_logged, "No known Wi-Fi found ❌"); break;
    case 13: strcpy(error_logged, "Camera init failed ❌"); break;
    case 14: strcpy(error_logged, "Web server started ✅"); break;
    case 15: strcpy(error_logged, "Wi-Fi connected ✅"); break;
    case 16: strcpy(error_logged, "Wi-Fi connect attempt failed ⚠️"); break;
    default: strcpy(error_logged, "Unknown state."); break;
  }
  Serial.println(error_logged);
}
