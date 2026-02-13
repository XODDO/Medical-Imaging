#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7789.h> // Hardware-specific library for ST7789
#include <SPI.h>


#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSerif9pt7b.h>
#include <Fonts/FreeMono9pt7b.h>
#include <Fonts/FreeMonoBold9pt7b.h>
#include <Fonts/FreeSansBold24pt7b.h> 
#include <Fonts/FreeSerifBold24pt7b.h>
#include <Fonts/FreeSansBoldOblique24pt7b.h>
#include <Fonts/FreeSansOblique24pt7b.h>

#include <Fonts/FreeSans12pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include <Fonts/FreeSans18pt7b.h>
#include <Fonts/FreeMono12pt7b.h>
#include <Fonts/FreeMonoBold12pt7b.h>
#include <Fonts/FreeMonoBold18pt7b.h>



  // For the breakout board, you can use any 2 or 3 pins.
  // These pins will also work for the 1.8" TFT shield.
  #define TFT_CS        2
  #define TFT_RST        14 // Or set to -1 and connect to Arduino RESET pin
  #define TFT_DC         1


  #define TFT_MOSI 13
  #define TFT_SCLK 15

// OPTION 1 (recommended) is to use the HARDWARE SPI pins, which are unique
// to each board and not reassignable. This is the fastest mode of operation 

//Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);

// OPTION 2 lets you interface the display using ANY TWO or THREE PINS,
// tradeoff being that performance is not as fast as hardware SPI above.

Adafruit_ST7789 LCD = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);


char DEVICE_ID[100] = "CLINOVATE CAVISCOPE";

char run_time_prompt[50] = "Camera On StandBy";

float p = 3.1415926;
bool taking_pic = false;

void setup(void) { delay(1000);
  Serial.begin(115200);
  

  LCD.init(76, 284); Serial.print(F("Display initialized"));
  LCD.setRotation(1);

  uint16_t time = millis(); uint32_t start_time = time;
  float dur = 0.0;
  LCD.fillScreen(ST77XX_BLACK);
  dur = (millis() - time)/1000.0;

  Serial.print(dur, 2); Serial.println(" milliseconds");
  delay(500);

 
  LCD.fillScreen(ST77XX_WHITE);
  LCD.setCursor(50, 40); LCD.setFont(&FreeSansBold12pt7b);
  LCD.setTextWrap(true);
  LCD.print(run_time_prompt);
  //testdrawtext(DEVICE_ID , ST77XX_WHITE);
  delay(2000);

  // tft print function!
 // tftPrintTest();



  //mediabuttons();
  delay(500);

if(!taking_pic){
  LCD.fillCircle(50, 40, 10, ST77XX_GREEN);
}

  LCD.setCursor(70, 40); LCD.setFont(&FreeSerif9pt7b);
  LCD.setTextColor(ST77XX_YELLOW);
  //LCD.setTextWrap(true);
  LCD.print(run_time_prompt);

  float duration = ((float)millis() - (float)start_time)/1000.0;

  Serial.println(); Serial.print("Booting done in "); Serial.print(duration); Serial.println("seconds"); Serial.println();
  delay(1000);
}

void loop() {
  /*
  LCD.invertDisplay(true);
  delay(500);
  LCD.invertDisplay(false);
  delay(500);
  */
}

void testlines(uint16_t color) {
  LCD.fillScreen(ST77XX_BLACK);
  for (int16_t x=0; x < LCD.width(); x+=6) {
    LCD.drawLine(0, 0, x, LCD.height()-1, color);
    delay(0);
  }
  for (int16_t y=0; y < LCD.height(); y+=6) {
    LCD.drawLine(0, 0, LCD.width()-1, y, color);
    delay(0);
  }

  LCD.fillScreen(ST77XX_BLACK);
  for (int16_t x=0; x < LCD.width(); x+=6) {
    LCD.drawLine(LCD.width()-1, 0, x, LCD.height()-1, color);
    delay(0);
  }
  for (int16_t y=0; y < LCD.height(); y+=6) {
    LCD.drawLine(LCD.width()-1, 0, 0, y, color);
    delay(0);
  }

  LCD.fillScreen(ST77XX_BLACK);
  for (int16_t x=0; x < LCD.width(); x+=6) {
    LCD.drawLine(0, LCD.height()-1, x, 0, color);
    delay(0);
  }
  for (int16_t y=0; y < LCD.height(); y+=6) {
    LCD.drawLine(0, LCD.height()-1, LCD.width()-1, y, color);
    delay(0);
  }

  LCD.fillScreen(ST77XX_BLACK);
  for (int16_t x=0; x < LCD.width(); x+=6) {
    LCD.drawLine(LCD.width()-1, LCD.height()-1, x, 0, color);
    delay(0);
  }
  for (int16_t y=0; y < LCD.height(); y+=6) {
    LCD.drawLine(LCD.width()-1, LCD.height()-1, 0, y, color);
    delay(0);
  }
}

void testdrawtext(char *text, uint16_t color) {
  LCD.setCursor(0, 0);
  LCD.setTextColor(color);
  LCD.setTextWrap(true);
  LCD.print(text);
}

void testfastlines(uint16_t color1, uint16_t color2) {
  LCD.fillScreen(ST77XX_BLACK);
  for (int16_t y=0; y < LCD.height(); y+=5) {
    LCD.drawFastHLine(0, y, LCD.width(), color1);
  }
  for (int16_t x=0; x < LCD.width(); x+=5) {
    LCD.drawFastVLine(x, 0, LCD.height(), color2);
  }
}

void testdrawrects(uint16_t color) {
  LCD.fillScreen(ST77XX_BLACK);
  for (int16_t x=0; x < LCD.width(); x+=6) {
    LCD.drawRect(LCD.width()/2 -x/2, LCD.height()/2 -x/2 , x, x, color);
  }
}

void testfillrects(uint16_t color1, uint16_t color2) {
  LCD.fillScreen(ST77XX_BLACK);
  for (int16_t x=LCD.width()-1; x > 6; x-=6) {
    LCD.fillRect(LCD.width()/2 -x/2, LCD.height()/2 -x/2 , x, x, color1);
    LCD.drawRect(LCD.width()/2 -x/2, LCD.height()/2 -x/2 , x, x, color2);
  }
}

void testfillcircles(uint8_t radius, uint16_t color) {
  for (int16_t x=radius; x < LCD.width(); x+=radius*2) {
    for (int16_t y=radius; y < LCD.height(); y+=radius*2) {
      LCD.fillCircle(x, y, radius, color);
    }
  }
}

void testdrawcircles(uint8_t radius, uint16_t color) {
  for (int16_t x=0; x < LCD.width()+radius; x+=radius*2) {
    for (int16_t y=0; y < LCD.height()+radius; y+=radius*2) {
      LCD.drawCircle(x, y, radius, color);
    }
  }
}

void testtriangles() {
  LCD.fillScreen(ST77XX_BLACK);
  uint16_t color = 0xF800;
  int t;
  int w = LCD.width()/2;
  int x = LCD.height()-1;
  int y = 0;
  int z = LCD.width();
  for(t = 0 ; t <= 15; t++) {
    LCD.drawTriangle(w, y, y, x, z, x, color);
    x-=4;
    y+=4;
    z-=4;
    color+=100;
  }
}

void testroundrects() {
  LCD.fillScreen(ST77XX_BLACK);
  uint16_t color = 100;
  int i;
  int t;
  for(t = 0 ; t <= 4; t+=1) {
    int x = 0;
    int y = 0;
    int w = LCD.width()-2;
    int h = LCD.height()-2;
    for(i = 0 ; i <= 16; i+=1) {
      LCD.drawRoundRect(x, y, w, h, 5, color);
      x+=2;
      y+=3;
      w-=4;
      h-=6;
      color+=1100;
    }
    color+=100;
  }
}

void tftPrintTest() {
  LCD.setTextWrap(false);
  LCD.fillScreen(ST77XX_BLACK);
  LCD.setCursor(0, 30);
  LCD.setTextColor(ST77XX_RED);
  LCD.setTextSize(1);
  LCD.println("Hello World!");
  LCD.setTextColor(ST77XX_YELLOW);
  LCD.setTextSize(2);
  LCD.println("Hello World!");
  LCD.setTextColor(ST77XX_GREEN);
  LCD.setTextSize(3);
  LCD.println("Hello World!");
  LCD.setTextColor(ST77XX_BLUE);
  LCD.setTextSize(4);
  LCD.print(1234.567);
  delay(1500);
  LCD.setCursor(0, 0);
  LCD.fillScreen(ST77XX_BLACK);
  LCD.setTextColor(ST77XX_WHITE);
  LCD.setTextSize(0);
  LCD.println("Hello World!");
  LCD.setTextSize(1);
  LCD.setTextColor(ST77XX_GREEN);
  LCD.print(p, 6);
  LCD.println(" Want pi?");
  LCD.println(" ");
  LCD.print(8675309, HEX); // print 8,675,309 out in HEX!
  LCD.println(" Print HEX!");
  LCD.println(" ");
  LCD.setTextColor(ST77XX_WHITE);
  LCD.println("Sketch has been");
  LCD.println("running for: ");
  LCD.setTextColor(ST77XX_MAGENTA);
  LCD.print(millis() / 1000);
  LCD.setTextColor(ST77XX_WHITE);
  LCD.print(" seconds.");
}

void mediabuttons() {
  // play
  LCD.fillScreen(ST77XX_BLACK);
  LCD.fillRoundRect(25, 10, 78, 60, 8, ST77XX_WHITE);
  LCD.fillTriangle(42, 20, 42, 60, 90, 40, ST77XX_RED);
  delay(500);
  // pause
  LCD.fillRoundRect(25, 90, 78, 60, 8, ST77XX_WHITE);
  LCD.fillRoundRect(39, 98, 20, 45, 5, ST77XX_GREEN);
  LCD.fillRoundRect(69, 98, 20, 45, 5, ST77XX_GREEN);
  delay(500);
  // play color
  LCD.fillTriangle(42, 20, 42, 60, 90, 40, ST77XX_BLUE);
  delay(50);
  // pause color
  LCD.fillRoundRect(39, 98, 20, 45, 5, ST77XX_RED);
  LCD.fillRoundRect(69, 98, 20, 45, 5, ST77XX_RED);
  // play color
  LCD.fillTriangle(42, 20, 42, 60, 90, 40, ST77XX_GREEN);
}
