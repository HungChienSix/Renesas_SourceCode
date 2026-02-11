#ifndef _SPI_H_
#define _SPI_H_

#include "stm32f4xx.h"

/* 定义管脚端口 */
#define LCD_SCK_GPIO_PORT   GPIOG
#define LCD_SCK_GPIO_PIN    GPIO_Pin_12
#define LCD_SCK_GPIO_CLK    RCC_AHB1Periph_GPIOG

#define LCD_MOSI_GPIO_PORT   GPIOD
#define LCD_MOSI_GPIO_PIN    GPIO_Pin_5
#define LCD_MOSI_GPIO_CLK    RCC_AHB1Periph_GPIOD

#define LCD_RES_GPIO_PORT   GPIOD
#define LCD_RES_GPIO_PIN    GPIO_Pin_4
#define LCD_RES_GPIO_CLK    RCC_AHB1Periph_GPIOD

#define LCD_DC_GPIO_PORT    GPIOD
#define LCD_DC_GPIO_PIN     GPIO_Pin_15
#define LCD_DC_GPIO_CLK     RCC_AHB1Periph_GPIOD

#define LCD_CS_GPIO_PORT    GPIOD
#define LCD_CS_GPIO_PIN     GPIO_Pin_1
#define LCD_CS_GPIO_CLK     RCC_AHB1Periph_GPIOD

#define LCD_BLK_GPIO_PORT    GPIOE
#define LCD_BLK_GPIO_PIN     GPIO_Pin_8
#define LCD_BLK_GPIO_CLK     RCC_AHB1Periph_GPIOE

/* 定义端口电平状态 */
#define LCD_SCK_Clr() GPIO_ResetBits(LCD_SCK_GPIO_PORT,LCD_SCK_GPIO_PIN)
#define LCD_SCK_Set() GPIO_SetBits(LCD_SCK_GPIO_PORT,LCD_SCK_GPIO_PIN)

#define LCD_MOSI_Clr() GPIO_ResetBits(LCD_MOSI_GPIO_PORT,LCD_MOSI_GPIO_PIN)
#define LCD_MOSI_Set() GPIO_SetBits(LCD_MOSI_GPIO_PORT,LCD_MOSI_GPIO_PIN)

#define LCD_RES_Clr() GPIO_ResetBits(LCD_RES_GPIO_PORT,LCD_RES_GPIO_PIN)
#define LCD_RES_Set() GPIO_SetBits(LCD_RES_GPIO_PORT,LCD_RES_GPIO_PIN)

#define LCD_DC_Clr() GPIO_ResetBits(LCD_DC_GPIO_PORT,LCD_DC_GPIO_PIN)
#define LCD_DC_Set() GPIO_SetBits(LCD_DC_GPIO_PORT,LCD_DC_GPIO_PIN)

#define LCD_CS_Clr() GPIO_ResetBits(LCD_CS_GPIO_PORT,LCD_CS_GPIO_PIN)
#define LCD_CS_Set() GPIO_SetBits(LCD_CS_GPIO_PORT,LCD_CS_GPIO_PIN)

#define LCD_BLK_Clr() GPIO_ResetBits(LCD_BLK_GPIO_PORT,LCD_BLK_GPIO_PIN)
#define LCD_BLK_Set() GPIO_SetBits(LCD_BLK_GPIO_PORT,LCD_BLK_GPIO_PIN)

/* 函数声明 */
void LCD_GPIOInit(void);
void LCD_WR_Bus(uint8_t dat);
void LCD_WR_REG(uint8_t reg);
void LCD_WR_DATA8(uint8_t dat);
void LCD_WR_DATA(uint16_t dat);

#endif



