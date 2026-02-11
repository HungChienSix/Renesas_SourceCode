//////////////////////////////////////////////////////////////////////////////////
//本程序只供学习使用，未经作者许可，不得用于其它任何用途
//中景园电子
//店铺地址：http://shop73023976.taobao.com
//
//  文 件 名   : main.c
//  版 本 号   : v1.0
//  作    者   : zhaojian
//  生成日期   : 2024-03-19
//  最近修改   : 
//  功能描述   : LCD SPI演示例程(STM32系列)
//               GND  电源地
//               VCC  3.3V电源
//               SCLK PA5
//               MOSI PA7
//               RES  PD2
//               DC   PB5
//               CS   PA4
//              ----------------------------------------------------------------
// 修改历史   :
// 日    期   :
// 作    者   :zhaojian
// 修改内容   :创建文件
//版权所有，盗版必究。
//Copyright(C) 中景园电子2024-03-19
//All rights reserved
//******************************************************************************/

#include "delay.h"
#include "sys.h"
#include "lcd.h"
#include "pic.h"

int main(void)
{
    delay_init();//延时函数初始化
    LCD_Init();
    while(1)
    {
        LCD_Fill(0, 0, LCD_W, LCD_H, WHITE);
        LCD_ShowPicture(11, 0, 105, 56, gImage_1);
        LCD_ShowString(10, 70, "1.44 TFT_LCD TEST", RED, WHITE, 12, 0);
        LCD_ShowString(7, 90, "RESOLUTION:128x128", RED, WHITE, 12, 0);
        LCD_ShowString(16, 110, "DRIVER IC:ST7735", RED, WHITE, 12, 0);
        delay_ms(1000);
        LCD_Fill(0, 0, LCD_W, LCD_H, WHITE);
        LCD_Fill(0, 0, 32, LCD_H, RED);
        LCD_Fill(32, 0, 64, LCD_H, GREEN);
        LCD_Fill(64, 0, 96, LCD_H, BLUE);
        LCD_Fill(96, 0, 128, LCD_H, YELLOW);
        delay_ms(1000);
        LCD_Fill(0, 0, LCD_W / 2, LCD_H / 2, RED);
        LCD_Fill(LCD_W / 2, 0, LCD_W, LCD_H / 2, GREEN);
        LCD_Fill(0, LCD_H / 2, LCD_W / 2, LCD_H, BLUE);
        LCD_Fill(LCD_W / 2, LCD_H / 2, LCD_W, LCD_H, WHITE);
        delay_ms(1000);
        LCD_Fill(0, 0, LCD_W, LCD_H, WHITE);
        LCD_ShowPicture(0, 0, 128, 128, gImage_2);
        LCD_ShowString(13, 110, "color depth 16bit", BLACK, WHITE, 12, 1);
        delay_ms(1000);
    }
}



