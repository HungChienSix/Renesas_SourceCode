#include <Arduino.h>
#include "lcd.h"
#include "pic.h"

void setup()
{
  LCD_Init();
}

void loop()
{
  LCD_Fill(0, 0, LCD_W, LCD_H, WHITE);
  LCD_ShowPicture(11, 0, 105, 56, gImage_1);
  LCD_ShowString(10, 70, "1.80 TFT_LCD TEST", RED, WHITE, 12, 0);
  LCD_ShowString(7, 90, "RESOLUTION:128x160", RED, WHITE, 12, 0);
  LCD_ShowString(16, 110, "DRIVER IC:ST7735", RED, WHITE, 12, 0);
  delay(1000);
  LCD_Fill(0, 0, LCD_W, LCD_H, WHITE);
  LCD_Fill(0, 0, 32, LCD_H, RED);
  LCD_Fill(32, 0, 64, LCD_H, GREEN);
  LCD_Fill(64, 0, 96, LCD_H, BLUE);
  LCD_Fill(96, 0, 128, LCD_H, YELLOW);
  delay(1000);
  LCD_Fill(0, 0, LCD_W / 2, LCD_H / 2, RED);
  LCD_Fill(LCD_W / 2, 0, LCD_W, LCD_H / 2, GREEN);
  LCD_Fill(0, LCD_H / 2, LCD_W / 2, LCD_H, BLUE);
  LCD_Fill(LCD_W / 2, LCD_H / 2, LCD_W, LCD_H, WHITE);
  delay(1000);
  LCD_Fill(0, 0, LCD_W, LCD_H, WHITE);
  LCD_ShowPicture(0, 0, 128, 128, gImage_2);
  LCD_ShowString(13, 110, "color depth 16bit", BLACK, WHITE, 12, 1);
  delay(1000);
}