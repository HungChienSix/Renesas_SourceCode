#ifndef FONTS_H
#define FONTS_H

#include <stdint.h>

typedef struct {
  const uint8_t width;
  const uint8_t height;
  const uint8_t bytes_per_row;  // 每行字节数，可以预计算
  const uint8_t *font_data;
} struFont_t;

typedef struct {
  const uint8_t UTF8_code[3]; // UTF-8编码
  const uint8_t *font_data;
} struFont_UTF_data_t;

typedef struct {
  const uint8_t width;
  const uint8_t height;
  const uint8_t bytes_per_row;  // 每行字节数，可以预计算
  const struFont_UTF_data_t *font_UTF_data;
} struFont_UTF_t;

/* ASCII字体 */
extern const struFont_t Font_8x16_consola;
extern const struFont_t Font_8x12_consola;
extern const struFont_t Font_8x16_times;
extern const struFont_t Font_8x12_times;

/* UTF-8汉字字体 */
extern const struFont_UTF_t Font_UTF_16x16_YuMincho;
extern const struFont_UTF_t Font_UTF_16x12_YuMincho;

/* 单色图片 */
extern const unsigned char gImage_apple[128];

/* RGB图片 */
extern const unsigned char gImage_RGB_163music[2048];
extern const unsigned char gImage_RGB_kugou[2048];
extern const unsigned char gImage_RGB_QQmusic[2048];

#endif
