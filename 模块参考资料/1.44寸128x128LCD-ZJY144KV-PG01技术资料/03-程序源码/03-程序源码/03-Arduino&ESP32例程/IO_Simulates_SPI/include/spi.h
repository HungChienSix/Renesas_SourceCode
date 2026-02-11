#ifndef _SPI_H_
#define _SPI_H_

#include <Arduino.h>

/* 定义管脚端口 */
#define SCK 48
#define MOSI 47
#define RES 12
#define DC 13
#define CS 14
#define BLK 38

/* 定义端口电平状态 */
#define LCD_SCK_Clr() digitalWrite(SCK, LOW)
#define LCD_SCK_Set() digitalWrite(SCK, HIGH)

#define LCD_MOSI_Clr() digitalWrite(MOSI, LOW)
#define LCD_MOSI_Set() digitalWrite(MOSI, HIGH)

#define LCD_RES_Clr() digitalWrite(RES, LOW)
#define LCD_RES_Set() digitalWrite(RES, HIGH)

#define LCD_DC_Clr() digitalWrite(DC, LOW)
#define LCD_DC_Set() digitalWrite(DC, HIGH)

#define LCD_CS_Clr() digitalWrite(CS, LOW)
#define LCD_CS_Set() digitalWrite(CS, HIGH)

#define LCD_BLK_Clr() digitalWrite(BLK, LOW)
#define LCD_BLK_Set() digitalWrite(BLK, HIGH)

/* 函数声明 */
void LCD_GPIOInit(void);
void LCD_WR_Bus(uint8_t dat);
void LCD_WR_REG(uint8_t reg);
void LCD_WR_DATA8(uint8_t dat);
void LCD_WR_DATA(uint16_t dat);

#endif



