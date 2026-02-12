/* V1.0 Screen */
/* V1.1 Screen : 添加屏幕的输入，实现外界输入控制屏幕 */
/* V1.2 Screen : 实现按键控制 */

#ifndef __SCREEN_H
#define __SCREEN_H

#include "st7735.h"
#include "fonts.h"

#define SCREEN_Quarter1 0x01
#define SCREEN_Quarter2 0x02
#define SCREEN_Quarter3 0x04
#define SCREEN_Quarter4 0x08

typedef enum SCREEN_Event{
	SCREEN_OK = 0,			// 绘制成功
	SCREEN_PARAM_ERROR, 	// 参数值错误
	SCREEN_CHAR_EXCEED,		// 字符超出范围
} SCREEN_Event_t;

// ASCII字体
extern const struFont_t Font_8x16_consolas;
extern const struFont_t Font_8x16_consolas_i;
extern const struFont_t Font_8x16_consolas_b;
extern const struFont_t Font_8x16_consolas_u;

extern const struFont_t Font_8x12_consolas;
extern const struFont_t Font_8x12_consolas_i;
extern const struFont_t Font_8x12_consolas_b;
extern const struFont_t Font_8x12_consolas_u;

// UTF-8汉字字体
extern const struFont_UTF_t Font_UTF_16x16_YuMincho;
extern const struFont_UTF_t Font_UTF_16x12_YuMincho;

// 单色图片
extern const unsigned char gImage_apple[128];

void SCREEN_FillScreen(SCREEN_Pixel_t Pixel_Set);
SCREEN_Event_t SCREEN_DrawPixel(int16_t x0, int16_t y0, SCREEN_Pixel_t Pixel_Set, SCREEN_Mode_t type);
SCREEN_Event_t SCREEN_DrawLine(int16_t x0, int16_t x1, int16_t y0, int16_t y1, SCREEN_Pixel_t Pixel_Set, SCREEN_Mode_t type);
SCREEN_Event_t SCREEN_DrawRectSolid(int16_t x0, int16_t x1, int16_t y0, int16_t y1, SCREEN_Pixel_t Pixel_Set, SCREEN_Mode_t type);
SCREEN_Event_t SCREEN_DrawRectHollow(int16_t x0, int16_t x1, int16_t y0, int16_t y1, SCREEN_Pixel_t Pixel_Set, SCREEN_Mode_t type);
SCREEN_Event_t SCREEN_DrawRoundRectHollow(int16_t x0, int16_t x1, int16_t y0, int16_t y1,uint8_t radius, SCREEN_Pixel_t Pixel_Set, SCREEN_Mode_t type);
SCREEN_Event_t SCREEN_DrawRoundRectSolid(int16_t x0, int16_t x1, int16_t y0, int16_t y1,uint8_t radius, SCREEN_Pixel_t Pixel_Set, SCREEN_Mode_t type);
SCREEN_Event_t SCREEN_DrawQuarArc(int16_t x0, int16_t y0, uint16_t r, uint8_t quadrant_mask, SCREEN_Pixel_t Pixel_Set, SCREEN_Mode_t type);
SCREEN_Event_t SCREEN_DrawQuarSector(int16_t x0, int16_t y0, uint16_t r, uint8_t quadrant_mask, SCREEN_Pixel_t Pixel_Set, SCREEN_Mode_t type);
SCREEN_Event_t SCREEN_DrawChar(int16_t x0, int16_t y0, char ch, const struFont_t *font, SCREEN_Pixel_t Pixel_Set, SCREEN_Mode_t type);
SCREEN_Event_t SCREEN_DrawString(int16_t x0, int16_t y0, const char *str, const struFont_t *font, SCREEN_Pixel_t Pixel_Set, SCREEN_Mode_t type);
SCREEN_Event_t SCREEN_DrawUTFChar(int16_t x0, int16_t y0, const char *utf8_char, const struFont_UTF_t *hz_font, SCREEN_Pixel_t Pixel_Set, SCREEN_Mode_t type);
SCREEN_Event_t SCREEN_DrawUTF8String(int16_t x0, int16_t y0, const char *utf8_str, const struFont_t *ascii_font, const struFont_UTF_t *hz_font, SCREEN_Pixel_t Pixel_Set, SCREEN_Mode_t type);
SCREEN_Event_t SCREEN_DrawImage(int16_t x0, int16_t y0, uint16_t width, uint16_t height, const uint8_t *image, SCREEN_Pixel_t Pixel_Set, SCREEN_Mode_t type);

#endif
