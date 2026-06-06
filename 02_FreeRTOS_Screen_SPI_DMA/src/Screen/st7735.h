/* V1.0 ST7735 Screen Controller Driver */
#ifndef ST7735_H
#define ST7735_H

#include <stdint.h>
#include <stddef.h>
#include "bsp_api.h"

/* 屏幕尺寸和偏移量 */
#define ST7735_XSTART     2
#define ST7735_YSTART     3
#define ST7735_WIDTH      128
#define ST7735_HEIGHT     128

/* Color definitions RGB565值 */
#define SCREEN_BLACK      0x0000
#define SCREEN_RED        0xF800
#define SCREEN_GREEN      0x07E0
#define SCREEN_BLUE       0x001F
#define SCREEN_YELLOW     0xFFE0
#define SCREEN_MAGENTA    0xF81F
#define SCREEN_CYAN       0x07FF
#define SCREEN_WHITE      0xFFFF
#define SCREEN_COLOR565(r, g, b) (((r & 0xF8) << 8) | ((g & 0xFC) << 3) | ((b & 0xF8) >> 3))

/* ST7735 命令 */
#define ST7735_SWREST   0x01
#define ST7735_SLPIN    0x10
#define ST7735_SLPOUT   0x11
#define ST7735_PTLON    0x12
#define ST7735_NORON    0x13
#define ST7735_INVOFF   0x20
#define ST7735_INVON    0x21
#define ST7735_GAMSET   0x26
#define ST7735_DISPOFF  0x28
#define ST7735_DISPON   0x29
#define ST7735_CASET    0x2A
#define ST7735_RASET    0x2B
#define ST7735_RAMWR    0x2C
#define ST7735_RGBSET   0x2D
#define ST7735_PTLAR    0x30
#define ST7735_SCRLAR   0x33
#define ST7735_TEOFF    0x34
#define ST7735_TEON     0x35
#define ST7735_MADCTL   0x36
#define ST7735_VSCSAD   0x37
#define ST7735_IDMOFF   0x38
#define ST7735_IDMON    0x39
#define ST7735_COLMOD   0x3A
#define ST7735_FRMCTR1  0xB1
#define ST7735_FRMCTR2  0xB2
#define ST7735_FRMCTR3  0xB3
#define ST7735_INVCTR   0xB4
#define ST7735_PWCTR1   0xC0
#define ST7735_PWCTR2   0xC1
#define ST7735_PWCTR3   0xC2
#define ST7735_PWCTR4   0xC3
#define ST7735_PWCTR5   0xC4
#define ST7735_VMCTR    0xC5
#define ST7735_VMOFCTR  0xC7
#define ST7735_NVFCTR1  0xD7
#define ST7735_NVFCTR2  0xD8
#define ST7735_NVFCTR3  0xD9
#define ST7735_NVFCTR4  0xDF
#define ST7735_NVFCTR5  0xFD
#define ST7735_GAMCTRP  0xE0
#define ST7735_GAMCTRN  0xE1
#define ST7735_GCV      0xFC

/* MADCTL命令参数 */
#define ST7735_MADCTL_MY    0x01
#define ST7735_MADCTL_MX    0x01
#define ST7735_MADCTL_MV    0x00
#define ST7735_MADCTL_ML    0x00
#define ST7735_MADCTL_RGB   0x01
#define ST7735_MADCTL_MH    0x00
#define ST7735_MADCTL_Parameter (ST7735_MADCTL_MY << 7 | ST7735_MADCTL_MX << 6 | ST7735_MADCTL_MV << 5 | ST7735_MADCTL_ML << 4 | ST7735_MADCTL_RGB << 3 | ST7735_MADCTL_MH << 2)

typedef enum {
    SCREEN_Nor = 0x00,
    SCREEN_Xor,
} SCREEN_Mode_t;

typedef uint16_t ST7735_Pixel_t;

/* 屏幕控制器接口函数（平台无关） */
fsp_err_t ST7735_Hardware_Init(void);
fsp_err_t ST7735_Init(void);
void      ST7735_Reset(void);
void      ST7735_WriteCmd(uint8_t cmd);
void      ST7735_WriteByte(uint8_t data);
int       ST7735_WriteData(uint8_t *data, size_t data_size, uint32_t timeout);  // F-10: 返回 0=成功, 非 0=SPI 错误
void      ST7735_SetAddressWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);

#endif /* ST7735_H */