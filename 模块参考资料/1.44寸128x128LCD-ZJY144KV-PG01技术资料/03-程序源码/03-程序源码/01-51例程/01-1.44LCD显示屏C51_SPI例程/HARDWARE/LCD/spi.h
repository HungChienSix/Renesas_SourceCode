#ifndef _SPI_H_
#define _SPI_H_

#include "reg51.h"

/*类型    定义*/
#define uint8_t unsigned char
#define uint16_t unsigned int
#define uint32_t unsigned long
/* 定义管脚端口 */
sbit LCD_SCK = P1 ^ 0;  // 时钟管脚
sbit LCD_MOSI = P1 ^ 1; // 数据管脚
sbit LCD_RES = P1 ^ 2;  // 复位管脚
sbit LCD_DC = P1 ^ 3;   // 数据/命令管脚
sbit LCD_CS = P1 ^ 4;   // 片选信号线
sbit LCD_BLK = P1 ^ 5;  // 背光控制管脚

/* 定义端口电平状态 */
#define LCD_SCK_Set() LCD_SCK = 1
#define LCD_SCK_Clr() LCD_SCK = 0

#define LCD_MOSI_Set() LCD_MOSI = 1
#define LCD_MOSI_Clr() LCD_MOSI = 0

#define LCD_RES_Set() LCD_RES = 1
#define LCD_RES_Clr() LCD_RES = 0

#define LCD_DC_Set() LCD_DC = 1
#define LCD_DC_Clr() LCD_DC = 0

#define LCD_CS_Set() LCD_CS = 1
#define LCD_CS_Clr() LCD_CS = 0

#define LCD_BLK_Set() LCD_BLK = 1
#define LCD_BLK_Clr() LCD_BLK = 0

/* 函数声明 */
void LCD_GPIOInit(void);
void LCD_WR_Bus(uint8_t dat);
void LCD_WR_REG(uint8_t reg);
void LCD_WR_DATA8(uint8_t dat);
void LCD_WR_DATA(uint16_t dat);

#endif





