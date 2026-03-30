#ifndef ST7735_H
#define ST7735_H

// 本工程使用的是 ST7735 128*128 1.44寸 的屏幕，由于不带字库显示中文略显复杂

// SCK --> P202 (SCI4)
// TXD --> P302 (SCI4)
// RES --> P311 (output initial low)
// DC  --> P312 (output initial low)
// CS  --> P907 (output initial low)
// BLK --> P211 (output initial low)

#include "hal_data.h"
#include <stdio.h>

typedef uint16_t SCREEN_Pixel_t;

// 屏幕尺寸和偏移量
#define ST7735_XSTART 2
#define ST7735_YSTART 3
#define ST7735_WIDTH  128
#define ST7735_HEIGHT 128

// Color definitions RGB565值
#define SCREEN_BLACK   0x0000
#define SCREEN_RED     0xF800
#define SCREEN_GREEN   0x07E0
#define SCREEN_BLUE    0x001F
#define SCREEN_YELLOW  0xFFE0
#define SCREEN_MAGENTA 0xF81F
#define SCREEN_CYAN    0x07FF
#define SCREEN_WHITE   0xFFFF
#define SCREEN_COLOR565(r, g, b) (((r & 0xF8) << 8) | ((g & 0xFC) << 3) | ((b & 0xF8) >> 3))

// ST7735 命令
#define ST7735_SWREST   0x01 // 软件重启
#define ST7735_SLPIN    0x10 // 进入睡眠
#define ST7735_SLPOUT   0x11 // 解除睡眠//
#define ST7735_PTLON    0x12 // 部分显示模式开启
#define ST7735_NORON    0x13 // 部分显示模式关闭 (Normal)
#define ST7735_INVOFF   0x20 // 反色关闭 (Normal)
#define ST7735_INVON    0x21 // 反色开启
#define ST7735_GAMSET   0x26 // gamma设置
#define ST7735_DISPOFF  0x28 // 关闭显示
#define ST7735_DISPON   0x29 // 开启显示
#define ST7735_CASET    0x2A // Col地址设置 //
#define ST7735_RASET    0x2B // Row地址设置 //
#define ST7735_RAMWR    0x2C // 屏幕RAM写入
#define ST7735_RGBSET   0x2D // 颜色深度转换的查找表设置
#define ST7735_PTLAR    0x30 // 部分显示模式地址设置
#define ST7735_SCRLAR   0x33 // 滚动区域设置
#define ST7735_TEOFF    0x34 // 撕裂效果线关闭
#define ST7735_TEON     0x35 // 撕裂效果线开启/设置
#define ST7735_MADCTL   0x36 // Memory Data Access Control (控制屏幕显示方向)
#define ST7735_VSCSAD   0x37 // 滚动区域开始地址
#define ST7735_IDMOFF   0x38 // 退出空闲模式
#define ST7735_IDMON    0x39 // 进入空闲模式 (可显示颜色减少)
#define ST7735_COLMOD   0x3A // 像素格式//

#define ST7735_FRMCTR1  0xB1 // 帧率设置 (In normal mode/ Full colors)
#define ST7735_FRMCTR2  0xB2 // 空闲模式帧率设置 (In Idle mode/ 8-colors)
#define ST7735_FRMCTR3  0xB3 // 部分显示模式帧率设置 (In Partial mode/ full colors)
#define ST7735_INVCTR   0xB4 // 反色控制
#define ST7735_PWCTR1   0xC0 // 功耗控制
#define ST7735_PWCTR2   0xC1 // 功耗控制
#define ST7735_PWCTR3   0xC2 // 功耗控制
#define ST7735_PWCTR4   0xC3 // 功耗控制
#define ST7735_PWCTR5   0xC4 // 功耗控制
#define ST7735_VMCTR    0xC5 // VCOM控制
#define ST7735_VMOFCTR  0xC7 // VCOM Offset Control
#define ST7735_NVFCTR1  0xD7 // NVM Enable Command
#define ST7735_NVFCTR2  0xD8 // Set Address for NVM
#define ST7735_NVFCTR3  0xD9 // Set Data for NVM
#define ST7735_NVFCTR4  0xDF // NVM Write Command
#define ST7735_NVFCTR5  0xFD // Custom Mode Enable Command
#define ST7735_GAMCTRP  0xE0 // Gamma ‘+’polarity Correction Characteristics Setting
#define ST7735_GAMCTRN  0xE1 // Gamma ‘-’polarity Correction Characteristics Setting
#define ST7735_GCV      0xFC // Gate Pump Clock Frequency Variable

// MADCT命令的参数 YXV-000-正向 111-旋转180
#define ST7735_MADCTL_MY  0x01
#define ST7735_MADCTL_MX  0x01
#define ST7735_MADCTL_MV  0x00
#define ST7735_MADCTL_ML  0x00 // 00是从上到下 01是从下到上
#define ST7735_MADCTL_RGB 0x01 // 00是RGB 01是BGR
#define ST7735_MADCTL_MH  0x00 // 00是从左到右 01是从右到左
#define ST7735_MADCTL_Parameter (ST7735_MADCTL_MY << 7 | ST7735_MADCTL_MX << 6 | ST7735_MADCTL_MV << 5 | ST7735_MADCTL_ML << 4 | ST7735_MADCTL_RGB << 3 | ST7735_MADCTL_MH << 2 )

// SPI发送超时 (10 MHz时钟)
#define SPI_CMD_TIMEOUT_us		10000
#define SPI_DATA_TIMEOUT_us		100000  

// 分区刷新宏
// #define ST7735_PARTIAL_REFRESH

typedef enum SCREEN_Mode{
	SCREEN_Nor = 0x00,  // 正常绘制：新颜色覆盖旧颜色
	SCREEN_Xor ,		// 异或模式：相同颜色则反色
} SCREEN_Mode_t;

void            SCREEN_Init(void);
void            SCREEN_FillScreen(SCREEN_Pixel_t Pixel_Set);
uint32_t        SCREEN_RefreshScreen(void);

SCREEN_Pixel_t  ReadPixel(int16_t x, int16_t y);
void            DrawPixel(int16_t x, int16_t y, SCREEN_Pixel_t Pixel_Set);
void            DrawHorLine(int16_t x0, int16_t x1, int16_t y, SCREEN_Pixel_t Pixel_Set, SCREEN_Mode_t type);
void            DrawVerLine(int16_t x, int16_t y0, int16_t y1, SCREEN_Pixel_t Pixel_Set, SCREEN_Mode_t type);

#endif
