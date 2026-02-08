#ifndef __SCREEN_UI_H
#define __SCREEN_UI_H

#include "screen.h"

// 按键结构体
typedef struct {
  int8_t location[2]; // 组件中心坐标
  uint8_t frame[3]; // 组件边框参数 长度,宽度,圆角半径
  char label[32]; // 组件标签文本
  struFont_t *ascii_font; // ASCII字体
  struFont_UTF_t *hz_font; // UTF字体

  uint8_t state ; // 组件状态 0x00-未按下 0xFF-按下
} struUI_Button_t;

// 文本提示框结构体
typedef struct {
  int8_t location[2];     // 提示框中心坐标
  uint8_t frame[2];       // 边框参数 长度,宽度
  char text[64];          // 提示文本内容
  struFont_t *ascii_font;   // ASCII字体
  struFont_UTF_t *hz_font;  // UTF字体
} struUI_Tooltip_t;


// 加载条结构体
typedef struct {
  int8_t location[2];     // 加载条中心坐标
  uint8_t frame[2];       // 边框参数 长度,宽度
  uint8_t progress;       // 进度值 0-100
} struUI_ProgressBar_t;


SCREEN_Event_t SCREEN_DrawButton(struUI_Button_t *button, SCREEN_Pixel_t Pixel_Set, SCREEN_Mode_t type);
SCREEN_Event_t SCREEN_DrawTooltip(struUI_Tooltip_t *tooltip, SCREEN_Pixel_t Pixel_Set, SCREEN_Pixel_t bg_color, SCREEN_Mode_t type);
SCREEN_Event_t SCREEN_DrawProgressBar(struUI_ProgressBar_t *bar, SCREEN_Pixel_t Pixel_Set, SCREEN_Pixel_t fill_color, SCREEN_Mode_t type);

#endif
