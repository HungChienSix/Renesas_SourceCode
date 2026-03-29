#ifndef FONTS_H
#define FONTS_H

// 图片使用 image2lcd 取模
// 字体使用 PCtoLCD2002 取模
// 汉字取模完成之后，借助 python convert_font.py 转化成适合本项目的代码

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

#endif
