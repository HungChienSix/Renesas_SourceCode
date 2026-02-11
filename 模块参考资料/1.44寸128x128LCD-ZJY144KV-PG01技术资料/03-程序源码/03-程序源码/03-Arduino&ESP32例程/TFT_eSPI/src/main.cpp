#include <Arduino.h>
#include "pic.h"
#include <TFT_eSPI.h>      // Hardware-specific library
TFT_eSPI tft = TFT_eSPI(); // Invoke custom library
static uint16_t screenWidth;
static uint16_t screenHeight;

void setup()
{
  Serial.begin(115200);
  tft.begin();
  tft.setRotation(0);
  screenWidth = tft.width();
  screenHeight = tft.height();
  tft.fillScreen(TFT_BLACK);
  tft.drawRect(0, 0, screenWidth, screenHeight, TFT_RED);
  delay(500);
  tft.fillScreen(TFT_RED);
  delay(1000);
  tft.fillScreen(TFT_GREEN);
  delay(1000);
  tft.fillScreen(TFT_BLUE);
  delay(1000);
  tft.fillScreen(TFT_WHITE);
  delay(1000);
}

void loop()
{
  tft.setTextColor(TFT_RED, TFT_WHITE, 0);
  tft.drawCentreString("Zhongjingyuan", 64, 0, 2);
  tft.setTextColor(TFT_GREEN, TFT_WHITE, 0);
  tft.drawCentreString("1.80 TFT_LCD TEST", 64, 20, 2);
  tft.drawCentreString("Resolution:128x160", 64, 40, 2);
  tft.setTextColor(TFT_BLUE, TFT_WHITE, 0);
  tft.drawCentreString("DRIVER IC:ST7735", 64, 60, 2);
  tft.setTextColor(TFT_BLACK, TFT_WHITE, 0);
  tft.drawCentreString("Support for rotation", 64, 80, 2);
  tft.drawCentreString("in 4 directions.", 64, 100, 2);
}


