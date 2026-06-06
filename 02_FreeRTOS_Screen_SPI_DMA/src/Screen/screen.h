/* V2.0 Screen - Upper Layer Abstract */

#ifndef SCREEN_H
#define SCREEN_H

#include "fonts.h"
#include "bsp_api.h"
#include "screen_config.h"

/* 根据驱动类型选择对应的驱动 */
#if SCREEN_DRIVER_TYPE == 0
    #include "st7735.h"
    #define SCREEN_HW_Init        ST7735_Hardware_Init
    #define SCREEN_ChipInit       ST7735_Init
    #define SCREEN_SetWindow     ST7735_SetAddressWindow
    #define SCREEN_Width         ST7735_WIDTH
    #define SCREEN_Height        ST7735_HEIGHT
    #define SCREEN_Pixel_t       ST7735_Pixel_t
    #define SCREEN_XSTART        ST7735_XSTART
    #define SCREEN_YSTART        ST7735_YSTART
    #define SCREEN_MADCTL_Parameter ST7735_MADCTL_Parameter
    #define SCREEN_WriteCmd        ST7735_WriteCmd
    #define SCREEN_WriteData       ST7735_WriteData
    #define SCREEN_CMD_RAMWR        ST7735_RAMWR

#elif SCREEN_DRIVER_TYPE == 1
    #include "st7789.h"
    #define SCREEN_HW_Init        ST7789_Hardware_Init
    #define SCREEN_ChipInit       ST7789_Init
    #define SCREEN_SetWindow     ST7789_SetAddressWindow
    #define SCREEN_Width         ST7789_WIDTH
    #define SCREEN_Height        ST7789_HEIGHT
    #define SCREEN_Pixel_t       ST7789_Pixel_t
    #define SCREEN_XSTART        ST7789_XSTART
    #define SCREEN_YSTART        ST7789_YSTART
    #define SCREEN_MADCTL_Parameter ST7789_MADCTL_Parameter
    #define SCREEN_WriteCmd        ST7789_WriteCmd
    #define SCREEN_WriteData       ST7789_WriteData
    #define SCREEN_CMD_RAMWR        ST7789_RAMWR

#else
    #error "Unknown SCREEN_DRIVER_TYPE"
#endif

/* 屏幕尺寸常量 (兼容旧代码) */
#define SCREEN_COLUMN_NUMBER  SCREEN_Width
#define SCREEN_LINE_NUMBER    SCREEN_Height

/* 分区刷新宏 */
#define SCREEN_PARTIAL_REFRESH

/* 圆角半径最大限制 */
#define SCREEN_MAX_ROUND_RADIUS 64

/* 圆弧点缓存数组最大大小 */
#define SCREEN_MAX_ARC_POINTS 128

/* F-09: UTF 字模表扫描安全上限 (避免表尾哨兵缺失时越界) */
#define SCREEN_FONT_UTF_MAX_ENTRIES 4096

/* 象限掩码 */
#define SCREEN_Quarter1 0x01
#define SCREEN_Quarter2 0x02
#define SCREEN_Quarter3 0x04
#define SCREEN_Quarter4 0x08

typedef enum SCREEN_Event {
    SCREEN_OK = 0,
    SCREEN_OUT,
    SCREEN_PARAM_ERROR,
    SCREEN_CHAR_EXCEED,
} SCREEN_Event_t;

/* 屏幕初始化 */
fsp_err_t SCREEN_Init(void);

/* 刷新函数 */
uint32_t SCREEN_Refresh(void);
void SCREEN_Refresh_Force(void);

/* 基础绘图 */
void SCREEN_Fill(SCREEN_Pixel_t pixel);
SCREEN_Pixel_t SCREEN_GetPixel(int16_t x, int16_t y);
void SCREEN_SetPixel(int16_t x, int16_t y, SCREEN_Pixel_t pixel);
void SCREEN_DrawHLine(int16_t x0, int16_t x1, int16_t y, SCREEN_Pixel_t pixel, SCREEN_Mode_t mode);
void SCREEN_DrawVLine(int16_t x, int16_t y0, int16_t y1, SCREEN_Pixel_t pixel, SCREEN_Mode_t mode);

/* 图形绘制 */
SCREEN_Event_t SCREEN_DrawPixel(int16_t x, int16_t y, SCREEN_Pixel_t pixel, SCREEN_Mode_t mode);
SCREEN_Event_t SCREEN_DrawLine(int16_t x0, int16_t x1, int16_t y0, int16_t y1, SCREEN_Pixel_t pixel, SCREEN_Mode_t mode);
SCREEN_Event_t SCREEN_DrawRect(int16_t x0, int16_t x1, int16_t y0, int16_t y1, SCREEN_Pixel_t pixel, SCREEN_Mode_t mode);
SCREEN_Event_t SCREEN_DrawRectHollow(int16_t x0, int16_t x1, int16_t y0, int16_t y1, SCREEN_Pixel_t pixel, SCREEN_Mode_t mode);
SCREEN_Event_t SCREEN_DrawRoundRect(int16_t x0, int16_t x1, int16_t y0, int16_t y1, uint8_t radius, SCREEN_Pixel_t pixel, SCREEN_Mode_t mode);
SCREEN_Event_t SCREEN_DrawRoundRectHollow(int16_t x0, int16_t x1, int16_t y0, int16_t y1, uint8_t radius, SCREEN_Pixel_t pixel, SCREEN_Mode_t mode);
SCREEN_Event_t SCREEN_DrawArc(int16_t x0, int16_t y0, uint16_t r, uint8_t quadrant, SCREEN_Pixel_t pixel, SCREEN_Mode_t mode);
SCREEN_Event_t SCREEN_DrawSector(int16_t x0, int16_t y0, uint16_t r, uint8_t quadrant, SCREEN_Pixel_t pixel, SCREEN_Mode_t mode);

/* 文字绘制 */
SCREEN_Event_t SCREEN_DrawChar(int16_t x, int16_t y, char ch, const struFont_t *font, SCREEN_Pixel_t pixel, SCREEN_Mode_t mode);
SCREEN_Event_t SCREEN_DrawString(int16_t x, int16_t y, const char *str, const struFont_t *font, SCREEN_Pixel_t pixel, SCREEN_Mode_t mode);
SCREEN_Event_t SCREEN_DrawUTFChar(int16_t x, int16_t y, const char *utf8, const struFont_UTF_t *font, SCREEN_Pixel_t pixel, SCREEN_Mode_t mode);
SCREEN_Event_t SCREEN_DrawUTFString(int16_t x, int16_t y, const char *utf8_str, const struFont_t *ascii, const struFont_UTF_t *hz, SCREEN_Pixel_t pixel, SCREEN_Mode_t mode);

/* 图片绘制 */
SCREEN_Event_t SCREEN_DrawImage(int16_t x, int16_t y, uint16_t w, uint16_t h, const uint8_t *img, SCREEN_Pixel_t pixel, SCREEN_Mode_t mode);
SCREEN_Event_t SCREEN_DrawRGBImage(int16_t x, int16_t y, uint16_t w, uint16_t h, const uint8_t *img);

#endif /* SCREEN_H */