#include "WiFi.h"
#include "esp_camera.h"
#include "esp_timer.h"
#include "img_converters.h"
#include "Arduino.h"
#include "soc/soc.h"           // Disable brownour problems
#include "soc/rtc_cntl_reg.h"  // Disable brownour problems
#include "driver/rtc_io.h"
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include <FS.h>

#include "Wire.h"

// Add near top of file
struct PhotoWriteJob {
  uint8_t *buf;
  size_t len;
};

// Photo File Name to save in SPIFFS
#define FILE_PHOTO "/photo.jpg"

void photoWriteTask(void * param) {
  PhotoWriteJob *job = (PhotoWriteJob*) param;
  if(!job) { vTaskDelete(NULL); return; }

  // Write to SPIFFS
  File file = SPIFFS.open(FILE_PHOTO, FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open file in writing mode (task)");
  } else {
    size_t written = file.write(job->buf, job->len);
    file.close();
    //float = 
    Serial.printf("Photo saved. Bytes written: %u\n", (unsigned)written); Serial.println();
  }
  // free the copy buffer
  free(job->buf);
  free(job); // free job struct
  vTaskDelete(NULL);
}
// Replace with your network credentials
//const char* ssid = "Caviscope";
//const char* password = "IntelliSys@2024_";

const char* ssid = "IntelliSys Pro 2025";
const char* password = "intel_cool@2025";

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

boolean takeNewPhoto = false;



// OV2640 camera module pins (CAMERA_MODEL_AI_THINKER)
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

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="stylesheet" href='https://cdn.jsdelivr.net/npm/bootstrap@4.6.2/dist/css/bootstrap.min.css'>
  <link rel="stylesheet" href='https://cdnjs.cloudflare.com/ajax/libs/font-awesome/4.7.0/css/font-awesome.min.css'>
  <style>
    body { text-align:center; }
    .vert { margin-bottom: 10%; }
    .hori{ margin-bottom: 0%; }
  </style>
</head>
<body>
  <div id="container p-2">
    <h1>CAVISCOPE PICTURE MANAGER</h1>
    <div class = 'row'>
    <div class = 'col-1 col-md-3'></div>
      <div class='col-10 col-md-6 alert alert-primary border border-primary'><i class='fa fa-exclamation-triangle fa-fw' ></i> It takes 5-10 seconds to capture a photo.</div>
    <div class = 'col-1 col-md-3'></div>
    </div>
    <p>
      <button class = 'btn btn-md btn-primary' onclick="rotatePhoto();"><i class='fa fa-undo fa-fw'></i>ROTATE PIC</button>
      <button class = 'btn btn-md btn-success' onclick="capturePhoto()"><i class='fa fa-camera fa-fw'></i>CAPTURE PHOTO</button>
      <button class = 'btn btn-md btn-info' onclick="location.reload();"><i class='fa fa-refresh fa-fw'></i>REFRESH PAGE</button>
    </p>
  </div>
  <div>
  <img src="saved-photo" id="photo" width="60%"></div>

</body>
<script>
  var deg = 0;
  function capturePhoto() {
    var xhr = new XMLHttpRequest();
    xhr.open('GET', "/capture", true);
    xhr.send();
  }
  function rotatePhoto() {
    var img = document.getElementById("photo");
    deg += 90;
    if(isOdd(deg/90)){ document.getElementById("container").className = "vert"; }
    else{ document.getElementById("container").className = "hori"; }
    img.style.transform = "rotate(" + deg + "deg)";
  }
  function isOdd(n) { return Math.abs(n % 2) == 1; }
</script>
</html>)rawliteral";

void setup() {
  // Serial port for debugging purposes
  Serial.begin(115200);

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print("Connecting to WiFi..."); Serial.println(ssid);
  }
  Serial.print("Successfully Connected to "); Serial.println(ssid);

  if (!SPIFFS.begin(true)) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    ESP.restart();
  }
  else {
    delay(500);
    Serial.println("SPIFFS mounted successfully");
  }

  // Print ESP32 Local IP Address
  Serial.print("Picture located at IP Address: http://");
  Serial.println(WiFi.localIP());

  // Turn-off the 'brownout detector'
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

  // OV2640 camera module
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
  config.xclk_freq_hz = 20000000; // 20000000
  config.pixel_format = PIXFORMAT_JPEG;

Serial.printf("psramFound=%d\n", psramFound());
Serial.printf("Free Heap: %u\n", ESP.getFreeHeap());
#ifdef ESP.getFreePsram
  Serial.printf("Free PSRAM: %u\n", ESP.getFreePsram());
#endif


  if (psramFound()) {
    config.frame_size = FRAMESIZE_HD; //FRAMESIZE_UXGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }
  // Camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    ESP.restart();
  }

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(200, "text/html", index_html);
  });

  server.on("/capture", HTTP_GET, [](AsyncWebServerRequest * request) {
    takeNewPhoto = true;
    request->send(200, "text/plain", "Taking Photo");
  });

  server.on("/saved-photo", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, FILE_PHOTO, "image/jpg", false);
  });

  // Start server
  server.begin();

}



void loop() {
  if (takeNewPhoto) {
    capturePhoto_very_fast();
    takeNewPhoto = false;
  }
  delay(1);
}



/*
  <nav aria-label="Page navigation example">
  <ul class="pagination">
    <li class='page-item'><a class='page-link' href='#'>Previous</a></li>
    <li class='page-item'><a class='page-link' href='#'>1</a></li>
    <li class='page-item'><a class='page-link' href='#'>2</a></li>
    <li class='page-item'><a class='page-link' href='#'>3</a></li>
    <li class='page-item'><a class='page-link' href='#'>Next</a></li>
    
  </ul>
</nav>
*/



// Check if photo capture was successful
bool checkPhoto( fs::FS &fs ) {
  File f_pic = fs.open( FILE_PHOTO );
  unsigned int pic_sz = f_pic.size();
  return ( pic_sz > 100 );
}

/*
Best-practice fix (copy & background-write)
When capture begins print fb->len and fb->format and how long writing took (millis). 
If fb->len is zero or very large/unexpected or writing time is >>100ms, that’s a clue.

Idea: capture the frame, immediately copy the JPEG bytes to a newly allocated RAM buffer, 
return the fb right away, then write that copy to SPIFFS from a dedicated task. 
This prevents camera DMA stalls while SPIFFS writes happen.

esp_camera_fb_get() → get frame buffer.

Immediately copy JPEG bytes to a malloc'd buffer (fast memcpy).

esp_camera_fb_return(fb) → return the original buffer as fast as possible to avoid DMA overflow.

Start photoWriteTask to write the copy to SPIFFS in background. The camera DMA is never blocked by the slow SPIFFS write.
*/

// replacement for capturePhotoSaveSpiffs()
void capturePhoto_very_fast(void) {
  camera_fb_t * fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Camera capture failed");
    return;
  }

  // quick sanity
  Serial.printf("Captured frame len=%u\n", (unsigned)fb->len);
  if (fb->len == 0) {
    esp_camera_fb_return(fb);
    return;
  }


  // Allocate job + buffer
  PhotoWriteJob *job = (PhotoWriteJob*) malloc(sizeof(PhotoWriteJob));
  if (!job) {
    Serial.println("Failed to allocate job");
    esp_camera_fb_return(fb);
    return;
  }
  job->len = fb->len;
  job->buf = (uint8_t*) malloc(job->len);
  if (!job->buf) {
    Serial.println("Failed to allocate buffer copy");
    free(job);
    esp_camera_fb_return(fb);
    return;
  }

  // Copy payload to RAM copy immediately (fast memcpy)
  memcpy(job->buf, fb->buf, job->len);

  // Return frame buffer immediately so DMA can reuse it
  esp_camera_fb_return(fb);

  // Spawn a short-lived task to write the copy to SPIFFS
  BaseType_t ok = xTaskCreate(
    photoWriteTask,      // task function
    "photoWriteTask",    // name
    4096,                // stack size
    job,                 // parameter
    1,                   // priority
    NULL                 // task handle
  );

  if (ok != pdPASS) {
    Serial.println("Failed to create photo write task, writing inline");
    // fallback: write inline (still uses copy, but blocks)
    File file = SPIFFS.open(FILE_PHOTO, FILE_WRITE);
    if (file) {
      file.write(job->buf, job->len);
      file.close();
      Serial.printf("Fallback write done: %u bytes\n", (unsigned)job->len);
    }
    free(job->buf);
    free(job);
  }
}

// Capture Photo and Save it to SPIFFS
void capturePhotoSaveSpiffs( void ) {
  camera_fb_t * fb = NULL; // pointer
  bool ok = 0; // Boolean indicating if the picture has been taken correctly

  do {
    // Take a photo with the camera
    Serial.println("Taking a photo...");

    fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("Camera capture failed");
      return;
    }

    // Photo file name
    Serial.printf("Picture file name: %s\n", FILE_PHOTO);
    File file = SPIFFS.open(FILE_PHOTO, FILE_WRITE);

    // Insert the data in the photo file
    if (!file) {
      Serial.println("Failed to open file in writing mode");
    }
    else {
      file.write(fb->buf, fb->len); // payload (image), payload length
      Serial.print("The picture has been saved in ");
      Serial.print(FILE_PHOTO);
      Serial.print(" - Size: ");
      Serial.print(file.size());
      Serial.println(" bytes");
    }
    // Close the file
    file.close();
    esp_camera_fb_return(fb);

    // check if file has been correctly saved in SPIFFS
    ok = checkPhoto(SPIFFS);
  } while ( !ok );
}