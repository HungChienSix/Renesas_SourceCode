#ifndef __FONTS_H__
#define __FONTS_H__

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

#endif  // __FONTS_H__