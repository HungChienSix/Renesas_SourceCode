#ifndef __SCREEN_UI_H
#define __SCREEN_UI_H

#include "screen.h"

// // 输入类型定义
// #define INPUT_TYPE_KEY      0    // 按键类型 (value: 0松开 1按下)
// #define INPUT_TYPE_KNOB     1    // 旋钮类型 (value: 0-255)
// #define INPUT_TYPE_JOYSTICK 2    // 摇杆类型 (value: 高4位x轴 低4位y轴)

// typedef struct {
//     uint8_t type;       // 输入的种类,是只有012的按键还是0-255的旋钮还是0-16和0-16的摇杆
//     uint8_t id;         // 输入的标记值,每个输入都有一个唯一的标记值,用于区分不同的输入
//     uint8_t value;      // 输入的值,对于按键0是松开1是按下,对于旋钮是0-255,对于摇杆高4位是x轴值低4位是y轴值
// } struINPUT_t;

// 按键结构体
typedef struct {
  int8_t location[2]; // 组件中心坐标
  uint8_t frame[3]; // 组件边框参数 长度,宽度,圆角半径
  char label[32]; // 组件标签文本
  struFont_t *ascii_font; // ASCII字体
  struFont_UTF_t *hz_font; // UTF字体
  SCREEN_Pixel_t color[3]; // 边框颜色,填充颜色,文本颜色
  uint8_t state ; // 组件状态 0x00-未按下 0xFF-按下
} struUI_Button_t;

// 文本提示框结构体
typedef struct {
  int8_t location[2];     // 提示框中心坐标
  uint8_t frame[2];       // 边框参数 长度,宽度
  char text[64];          // 提示文本内容
  struFont_t *ascii_font;   // ASCII字体
  struFont_UTF_t *hz_font;  // UTF字体
  SCREEN_Pixel_t color[2]; // 背景颜色，文本颜色
} struUI_Tooltip_t;

// 加载条结构体
typedef struct {
  int8_t location[2];     // 加载条中心坐标
  uint8_t frame[2];       // 边框参数 长度,宽度
  uint8_t progress;       // 进度值 0-100
} struUI_ProgressBar_t;

// // 歌曲列表页面结构体
// typedef struct {
//     uint8_t selected_song_id;   // 当前选中的歌曲ID
//     uint8_t page_id;            // 当前页码（用于翻页）
// } struUI_Page1_t;

// void Screen_GetInput(struINPUT_t *input);
// void Screen_ClearInput(struINPUT_t *input);

SCREEN_Event_t SCREEN_DrawButton(struUI_Button_t *button, SCREEN_Mode_t type);
SCREEN_Event_t SCREEN_DrawTooltip(struUI_Tooltip_t *tooltip);
SCREEN_Event_t SCREEN_DrawProgressBar(struUI_ProgressBar_t *bar, SCREEN_Pixel_t fill_color, SCREEN_Mode_t type);

#endif
