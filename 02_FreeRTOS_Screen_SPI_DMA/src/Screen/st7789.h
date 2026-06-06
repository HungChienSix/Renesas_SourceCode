/* V1.0 ST7789 Screen Controller Driver */
#ifndef ST7789_H
#define ST7789_H

#include <stdint.h>
#include <stddef.h>
#include "bsp_api.h"

/* 屏幕尺寸和偏移量 */
#define ST7789_XSTART     0
#define ST7789_YSTART     0
#define ST7789_WIDTH      240
#define ST7789_HEIGHT     320

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

/* ST7789 命令 */
#define ST7789_SWREST   0x01
#define ST7789_SLPIN    0x10
#define ST7789_SLPOUT   0x11
#define ST7789_PTLON    0x12
#define ST7789_NORON    0x13
#define ST7789_INVOFF   0x20
#define ST7789_INVON    0x21
#define ST7789_GAMSET   0x26
#define ST7789_DISPOFF  0x28
#define ST7789_DISPON   0x29
#define ST7789_CASET    0x2A
#define ST7789_RASET    0x2B
#define ST7789_RAMWR    0x2C
#define ST7789_RGBSET   0x2D
#define ST7789_PTLAR    0x30
#define ST7789_SCRLAR   0x33
#define ST7789_TEOFF    0x34
#define ST7789_TEON     0x35
#define ST7789_MADCTL   0x36
#define ST7789_VSCSAD   0x37
#define ST7789_IDMOFF   0x38
#define ST7789_IDMON    0x39
#define ST7789_COLMOD   0x3A
#define ST7789_FRMCTR1  0xB1
#define ST7789_FRMCTR2  0xB2
#define ST7789_FRMCTR3  0xB3
#define ST7789_INVCTR   0xB4
#define ST7789_PWCTR1   0xC0
#define ST7789_PWCTR2    0xC1
#define ST7789_PWCTR3    0xC2
#define ST7789_PWCTR4    0xC3
#define ST7789_PWCTR5    0xC4
#define ST7789_VMCTR    0xC5
#define ST7789_PWCTR6    0xC6
#define ST7789_VMOFCTR  0xC7
#define ST7789_GAMCTRP  0xE0
#define ST7789_GAMCTRN  0xE1

/* MADCTL命令参数 - 横屏设置 */
#define ST7789_MADCTL_MY    0x00
#define ST7789_MADCTL_MX    0x00
#define ST7789_MADCTL_MV    0x00
#define ST7789_MADCTL_ML    0x00
#define ST7789_MADCTL_RGB   0x00
#define ST7789_MADCTL_MH    0x00
#define ST7789_MADCTL_Parameter (ST7789_MADCTL_MY << 7 | ST7789_MADCTL_MX << 6 | ST7789_MADCTL_MV << 5 | ST7789_MADCTL_ML << 4 | ST7789_MADCTL_RGB << 3 | ST7789_MADCTL_MH << 2)

typedef enum {
    SCREEN_Nor = 0x00,
    SCREEN_Xor,
} SCREEN_Mode_t;

typedef uint16_t ST7789_Pixel_t;

/* 屏幕控制器接口函数（平台无关） */
fsp_err_t ST7789_Hardware_Init(void);
fsp_err_t ST7789_Init(void);
void      ST7789_Reset(void);
void      ST7789_WriteCmd(uint8_t cmd);
void      ST7789_WriteByte(uint8_t data);
int       ST7789_WriteData(uint8_t *data, size_t data_size, uint32_t timeout);  // F-10: 返回 0=成功, 非 0=SPI 错误
void      ST7789_SetAddressWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);

#endif /* ST7789_H */