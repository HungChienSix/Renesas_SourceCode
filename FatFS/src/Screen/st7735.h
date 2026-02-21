#ifndef ST7735_H
#define ST7735_H

#include "hal_data.h"

#define ST7735 

// GPIO and Peripheral Setting
#define ST7735_SPI hspi1

#define TFT_RES_Pin     BSP_IO_PORT_05_PIN_11
#define TFT_DC_Pin      BSP_IO_PORT_08_PIN_04
#define TFT_CS_Pin 	    BSP_IO_PORT_08_PIN_03
#define TFT_BLK_Pin 	BSP_IO_PORT_00_PIN_02

// Screen Size
#define ST7735_XSTART 2
#define ST7735_YSTART 1
#define ST7735_WIDTH  128
#define ST7735_HEIGHT 128

// Screen Direction
#define ST7735_ROTATION 2

// Color Mode: RGB or BGR
#define ST7735_MADCTL_RGB 0x00
#define ST7735_MADCTL_BGR 0x08
#define ST7735_MADCTL_MODE ST7735_MADCTL_BGR

// Color Inverse: 0=NO, 1=YES
#define ST7735_INVERSE 0

// Color definitions RGB565值
#define SCREEN_BLACK   0x0000
#define SCREEN_BLUE    0x001F
#define SCREEN_RED     0xF800
#define SCREEN_GREEN   0x07E0
#define SCREEN_CYAN    0x07FF
#define SCREEN_MAGENTA 0xF81F
#define SCREEN_YELLOW  0xFFE0
#define SCREEN_WHITE   0xFFFF
#define SCREEN_COLOR565(r, g, b) (((r & 0xF8) << 8) | ((g & 0xFC) << 3) | ((b & 0xF8) >> 3))

#define ST7735_SLPOUT   0x11
#define ST7735_FRMCTR1  0xB1
#define ST7735_FRMCTR2  0xB2
#define ST7735_FRMCTR3  0xB3
#define ST7735_INVCTR   0xB4
#define ST7735_PWCTR1   0xC0
#define ST7735_PWCTR2   0xC1
#define ST7735_PWCTR3   0xC2
#define ST7735_PWCTR4   0xC3
#define ST7735_PWCTR5   0xC4
#define ST7735_VMCTR1   0xC5
#define ST7735_COLMOD   0x3A
#define ST7735_GMCTRP1  0xE0
#define ST7735_GMCTRN1  0xE1
#define ST7735_NORON    0x13
#define ST7735_DISPON   0x29
#define ST7735_CASET    0x2A
#define ST7735_RASET    0x2B
#define ST7735_RAMWR    0x2C
#define ST7735_INVOFF   0x20
#define ST7735_INVON    0x21

#define ST7735_MADCTL     0x36
#define ST7735_MADCTL_MX  0x40
#define ST7735_MADCTL_MY  0x80
#define ST7735_MADCTL_MV  0x20

// SPI发送超时
#define ST7735_SPI_CMD_TIMEOUT		100
#define ST7735_SPI_DATA_TIMEOUT		500

// 分区刷新宏 
#define ST7735_PARTIAL_REFRESH

typedef uint16_t SCREEN_Pixel_t;

typedef enum SCREEN_Mode{
	SCREEN_Nor = 0x00,// 正常显示,会被遮挡
	SCREEN_Xor = 0xFF,// 异或显示,遇到遮挡会反色
} SCREEN_Mode_t;

/**
 * @brief SPI传输错误类型
 */
typedef enum ST7735_SPI_Error{
	ST7735_SPI_OK = 0,         // SPI传输成功
	ST7735_SPI_TIMEOUT,         // SPI传输超时
	ST7735_SPI_BUSY,           // SPI设备忙碌
	ST7735_SPI_ERROR,           // SPI传输错误
} ST7735_SPI_Error_t;

/**
 * @brief 屏幕状态结构体
 * @note 用于记录屏幕运行状态、性能指标和配置信息
 */
typedef struct {
	/* 刷新状态标志 */
	uint8_t is_refreshing;           // 是否正在刷新屏幕 (0=空闲, 1=刷新中)

	/* 刷新耗时记录 */
	uint32_t refresh_time_ms;        // 刷新耗时

	/* 刷新间隔记录 */
	uint32_t refresh_interval_ms;    // 刷新间隔：从上次刷新结束到本次刷新开始
	
	float refresh_rate_fps;         // 刷新帧率（帧每秒）

} struSCREEN_state_t;

// 函数声明
void SPI_ST7735_Init(void);
void SCREEN_Init(void);
SCREEN_Pixel_t ReadPixel(int16_t x, int16_t y);
void DrawPixel(int16_t x, int16_t y, SCREEN_Pixel_t Pixel_Set);
void DrawHorLine(int16_t x0, int16_t x1, int16_t y, SCREEN_Pixel_t Pixel_Set, SCREEN_Mode_t type);
void DrawVerLine(int16_t x, int16_t y1, int16_t y2, SCREEN_Pixel_t Pixel_Set, SCREEN_Mode_t type);
void FillScreen(SCREEN_Pixel_t Pixel_Set);
void SCREEN_RefreshScreen(void);
void ST7735_SPI_Reset(void);  // SPI错误恢复函数

// 辅助函数
uint32_t SCREEN_GetRefreshTime();
uint32_t SCREEN_GetRefreshIntervalTime();
float SCREEN_GetFPS();

#endif
