/*********
    
    CAVISCOPE v1

*********/
//10TH AUG 2025

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
//#include "SD_MMC.h"  // SD Card ESP32
#include <EEPROM.h>  // read and write from flash memory
// define the number of bytes you want to access
//#define EEPROM_SIZE 1




//  BIG SCREEN
//#include "Adafruit_GFX.h"
//#include "Adafruit_ILI9341.h"

// For the Adafruit shield, these are the default.
const uint8_t TFT_DC = 1;
const uint8_t TFT_CS = 2;  //39; //5;//39;
const uint8_t RST = 13;    // 4;

const uint8_t TFT_MOSI = 15;
const uint8_t TFT_MISO = 12;
const uint8_t TFT_CLK = 14;


//Adafruit_ILI9341 LCD = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_MOSI, TFT_CLK, RST, TFT_MISO);  // VERY SLOOOW
//  Adafruit_ILI9341 LCD(TFT_CS, TFT_DC, RST); // HARDWARE SPI //12X faster

const uint8_t flash_LED = 4;

// Replace with your network credentials
//const char* ssid = "MMU Weather";
//const char* password = "IntelliSys@2024_";
  const char* ssid = "IntelliSys Online";
  const char* password = "qwerty0976_";

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

boolean takeNewPhoto = false;
bool sent = false;

// Photo File Name to save on SD
#define FILE_PHOTO "/photo.jpg"

// OV2640 camera module pins (CAMERA_MODEL_AI_THINKER)
#define PWDN_GPIO_NUM 32
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM 0
#define SIOD_GPIO_NUM 26
#define SIOC_GPIO_NUM 27
#define Y9_GPIO_NUM 35
#define Y8_GPIO_NUM 34
#define Y7_GPIO_NUM 39
#define Y6_GPIO_NUM 36
#define Y5_GPIO_NUM 21
#define Y4_GPIO_NUM 19
#define Y3_GPIO_NUM 18
#define Y2_GPIO_NUM 5
#define VSYNC_GPIO_NUM 25
#define HREF_GPIO_NUM 23
#define PCLK_GPIO_NUM 22

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
  <div id="container" class = 'container'>
    <h1>CAVISCOPE PICTURE MANAGER</h1>
    <div class = 'row'>
    <div class = 'col-1 col-md-3'></div>
      <div class='col-10 col-md-6 alert alert-primary border border-primary'><i class='fa fa-exclamation-triangle fa-fw' ></i> It takes 5-10 seconds to capture a photo.</div>
    <div class = 'col-1 col-md-3'></div>
    </div>
    <p class = 'py-2 py-md-1'>
      
      <button class = 'btn btn-md btn-success mb-1' onclick="capturePhoto()"><i class='fa fa-camera fa-fw'></i>CAPTURE PHOTO</button>
      <button class = 'btn btn-md btn-primary mb-1' onclick="rotatePhoto();"><i class='fa fa-undo fa-fw'></i>ROTATE PHOTO</button>
      <button class = 'btn btn-md btn-info mb-1' onclick="location.reload();"><i class='fa fa-refresh fa-fw'></i>RELOAD IMAGE</button>
      <button class = 'btn btn-md btn-secondary mb-1' onclick="downloadImage()"><i class='fa fa-download fa-fw'></i>SAVE IMAGE</button>
    </p>
  </div>
  <div>
  <img src="saved-photo" id="photograph" width="65%"></div>



</body>
<script>
  var deg = 0;
  function capturePhoto() {
    var xhr = new XMLHttpRequest();
    xhr.open('GET', "/capture", true);
    xhr.send();
    //location.reload(); // to load the most recent image
  }

  function rotatePhoto() {
    var img = document.getElementById("photograph");
    deg += 90;
    img.style.transform = "rotate(" + deg + "deg)";
    if(isOdd(deg/90)){ document.getElementById("container").className = "vert"; }
    else{ document.getElementById("container").className = "hori"; }
    
  }

  function isOdd(n) { return Math.abs(n % 2) == 1; }

  function downloadImage() {
      const img = document.getElementById("photograph");
      const imageUrl = img.src;
      const fileName = imageUrl.split('/').pop();

      const link = document.createElement('a');
      link.href = imageUrl;
      link.download = fileName;
      document.body.appendChild(link);
      link.click();
      document.body.removeChild(link);
    }
</script>
</html>)rawliteral";

int pictureNumber = 0;

void setup() {

  // Serial port for debugging purposes
  Serial.begin(115200); Serial.println("BOOTING CAVISCOPE...\n"); 
  delay(1000);

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print("Connecting to  WiFi Network: "); Serial.println(ssid);
  }
  Serial.print("Successfully Connected to");
  Serial.println(ssid);



  // Print ESP32 Local IP Address
  Serial.print("IP Address: http://");
  Serial.println(WiFi.localIP());


/*
  Serial.println("Starting SD Card...");
  if (!SD_MMC.begin()) {
    Serial.println("SD Card Mount Failed");
    //return;
  }

  uint8_t cardType = SD_MMC.cardType();
  if (cardType == CARD_NONE) {
    Serial.println("No SD Card attached");
    //return;
  }

  */

  Serial.println("Starting SPIFFS...");
  if (!SPIFFS.begin(true)) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    ESP.restart();
    delay(100);
  } else {
    delay(500);
    Serial.println("SPIFFS mounted successfully");
  }
  // Turn-off the 'brownout detector'
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

  Serial.println("Initializing Camera...");
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
  config.xclk_freq_hz = 10000000; // = 20000000;     // drop to 10000000 if errors persist
  config.pixel_format = PIXFORMAT_JPEG;  // JPEG images are compressed and easier to handle.

  config.fb_location = CAMERA_FB_IN_PSRAM; // Ensure your ESP32-CAM has PSRAM and it’s initialized:
 // config.grab_mode    = CAMERA_GRAB_LATEST;

 Serial.printf("Free heap: %d\n", ESP.getFreeHeap());
 Serial.printf("Free PSRAM: %d\n", ESP.getFreePsram());

  Serial.println("Starting PSRAM...");

  if (psramFound()) {
    //config.frame_size = FRAMESIZE_UXGA; // 1600 x 1200
    config.frame_size = FRAMESIZE_HD; //; // 1280 x 720
    config.jpeg_quality = 8; // 10 out of 63
    config.fb_count = 1; // 2
    Serial.println("\tPSRAM found."); 
  } else {
   // config.frame_size = FRAMESIZE_SVGA; // 800X600
    config.frame_size = FRAMESIZE_QVGA; // 320X240  // Avoid UXGA[1600x1200] or SXGA[1280x1024] unless absolutely necessary.
    config.jpeg_quality = 12;
    config.fb_count = 1;
    Serial.println("\tPSRAM not found. Streaming may failure.");
  }
/*
  Frame Size: SVGA (800x600)
  JPEG Quality: 10
  fb_count: 2
  RAM Used: ~2.3MB in PSRAM
*/

  // Camera init
  Serial.println("Now initalizing camera...");
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    ESP.restart();
  }

    Serial.print("Image Frame Size set to:"); Serial.println(config.frame_size);
    Serial.print("Image JPEG Quality set to:"); Serial.println(config.jpeg_quality);
    Serial.print("Image FB Count set to:"); Serial.println(config.fb_count);



 Serial.println("Setting Web page Events...");
  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(200, "text/html", index_html);
  });

  server.on("/capture", HTTP_GET, [](AsyncWebServerRequest* request) {
    takeNewPhoto = true;
    request->send(200, "text/plain", "Taking Photo");
  });


  server.on("/saved-photo", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(SPIFFS, FILE_PHOTO, "image/jpg", false);
    sent = true;
  });


  // Start server
  Serial.println("Starting Web Server...");
  server.begin();
  Serial.println("Web Server Started!");

/* // If EEPROM is needed, move it out of the main task or initialize it after camera init and in a different scope.
// Using EEPROM.begin() together with camera streaming can cause memory issues. If you're not using EEPROM, remove or delay its initialization:

  Serial.println("Starting EEPROM...");
  EEPROM.begin(EEPROM_SIZE);

  if (EEPROM.read(1) >= 255) {
    EEPROM.write(1, 0);
    EEPROM.commit();
  }
  pictureNumber = EEPROM.read(1) + 1;
*/
  pinMode(flash_LED, OUTPUT);
  digitalWrite(flash_LED, 0);

  Serial.print("Booting LCD in Software Serial...");
  //LCD.begin();
  //LCD.setRotation(3); // for the 2.0-2.8" screen
  //BootScreen();

  Serial.println("Done Booting!");
  Serial.println();
  delay(1000);
}


// Use a lightweight web stream sketch with optimized buffer management. 
//Avoid handling SD saving + browser streaming in the same task unless you're tightly managing buffers.

void loop() { 
  if (takeNewPhoto) { takeNewPhoto = false;
        capturePhoto();  // and load it into SPIFFS();

//


        //send over WiFi();
        //save to SD();

        if (sent) Serial.println("Image Sent via WiFi!");
        
  } 
  delay(250); // Serial.println();
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
bool checkPhoto(fs::FS& fs) {
  File f_pic = fs.open(FILE_PHOTO);
  unsigned int pic_size = f_pic.size();
  return (pic_size > 100);
}



camera_fb_t* fb = NULL;  // pointer

uint8_t shots = 0;

// Capture Photo and Save it to SPIFFS
void capturePhoto(void) {

  bool taken_well = false;  // Boolean indicating if the picture has been taken correctly
  shots = 0;

  do {
    Serial.println();
    Serial.println();
    Serial.print("Taking a photo...");
    Serial.println(shots + 1);  // Take a photo with the camera

    digitalWrite(flash_LED, 1);
    Serial.println("FLASHLIGHT ON");
    //delay(50);
    fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("Camera capture failed");
      digitalWrite(flash_LED, 0);
      return;  // eliminate return to allow other parts of the code to run and replace with an error address code

    }

    else {
      Serial.println("Photo successfully taken!");
      digitalWrite(flash_LED, 0);

      //Serial.print("Number of pics so far in EEPROM:");
      //Serial.println(pictureNumber);

      load_into_SPIFFS();
      esp_camera_fb_return(fb);
      taken_well = checkPhoto(SPIFFS);

     // flash(1, 0);

      shots++;
     // delay(50);
      if (shots >= 5) {
        Serial.println("Retakes ziwedeyo");
        break;
      }  // quit after 5 attempts
    }
  } while (!taken_well);
/*
  if (taken_well) {
    pictureNumber++;
    EEPROM.write(1, pictureNumber);
    EEPROM.commit();
    Serial.println("Picture count saved into EEPROM!");
  }*/
}


void load_into_SPIFFS() {
  // Photo file name
  Serial.printf("Picture file name: %s\n", FILE_PHOTO);
  File file = SPIFFS.open(FILE_PHOTO, FILE_WRITE);

  // Insert the data in the photo file
  if (!file) {
    Serial.println("Failed to open SPIFFS for writing image");
  } else {
    file.write(fb->buf, fb->len);  // payload (image), payload length
    Serial.print("The picture has been saved into SPIFFS as ");
    Serial.print(FILE_PHOTO);
    Serial.print(" with Size: ");
    Serial.print(file.size() / 1000.0);
    Serial.println(" kB");
  }
  // Close the file
  file.close();
}

/*
void save_to_SD() {

  // Path where new picture will be saved in SD Card
  String path = "/picture" + String(pictureNumber) + ".jpg";

  fs::FS& fs = SD_MMC;
  Serial.printf("Picture file name: %s\n", path.c_str());

  File file = fs.open(path.c_str(), FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open SD card for writing");
    // return;
  } else {
    file.write(fb->buf, fb->len);  // payload (image), payload length
    Serial.printf("Saved file to SD Card path: %s\n", path.c_str());
  //  EEPROM.write(1, pictureNumber);
  //  EEPROM.commit();
  }
  file.close();
}
*/
void flash(uint8_t times, uint8_t delay_setting) {

  for (int xy = 0; xy < times; xy++) {
    digitalWrite(flash_LED, HIGH);

    if (delay_setting == 0) delay(50);
    else if (delay_setting == 1) delay(100);
    else if (delay_setting == 2) delay(500);

    digitalWrite(flash_LED, LOW);
    if (times > 1) delay(50);
  }
}




char origin[100] = "This Technology is Designed and Manufactured in Uganda";

uint32_t screen_boot_dur = 0, screen_boot_start = 0;

char boot_dur[20] = "";
/*
void BootScreen() {
  Serial.println("Running Boot Sequence...");
  screen_boot_start = millis();
  LCD.fillScreen(ILI9341_BLACK);
  LCD.setTextColor(ILI9341_WHITE);
  LCD.setCursor(10, 90);
  LCD.setTextSize(5);
  LCD.print("IntelliSys");

  // BACK LIGHT
  //    pinMode(backlight_, OUTPUT); digitalWrite(backlight_, HIGH);

  LCD.setTextColor(ILI9341_LIGHTGREY);
  LCD.setCursor(140, 210);
  LCD.setTextSize(2);
  LCD.print("2025");
  Serial.println("IntelliSys Screen Done");

  delay(700);

  LCD.fillScreen(ILI9341_NAVY);

  LCD.setTextColor(ILI9341_WHITE);
  LCD.setCursor(90, 10);
  LCD.setTextSize(3);
  LCD.print("CAVISCOPE");
  delay(500);


  LCD.setTextSize(2);
  LCD.setCursor(80, 60);
  LCD.print("Non-invasive");
  LCD.setCursor(38, 90);
  LCD.print("Cervical Cancer");
  LCD.setCursor(75, 140);
  LCD.print("Detection Device");

  delay(200);
  Serial.println("Now to the Uganda FLAG");

  for (int i = 0; i <= 320; i += 4) {
    LCD.fillRect(0, 160, i, 30, ILI9341_BLACK);
  }
  delay(50);

  for (int i = 0; i <= 320; i += 4) {
    LCD.fillRect(0, 190, i, 30, ILI9341_YELLOW);
  }
  delay(50);

  for (int i = 0; i <= 320; i += 4) {
    LCD.fillRect(0, 220, i, 30, ILI9341_RED);
  }
  delay(50);



  LCD.setTextSize(1);
  LCD.setTextColor(ILI9341_WHITE);
  LCD.setCursor(1, 225);
  LCD.print(origin);
  delay(1000);
  
 
  delay(1000);

  screen_boot_dur = (millis() - screen_boot_start) / 1000;
  ltoa(screen_boot_dur, boot_dur, 10);
  strcat(boot_dur, " seconds");

  LCD.setTextSize(1);
  LCD.setTextColor(ILI9341_WHITE);
  LCD.setCursor(20, 225);
  LCD.print(boot_dur);


  Serial.print("TFT Boot Duration: ");
  Serial.print(screen_boot_dur);
  Serial.println(" seconds");
}
*/
