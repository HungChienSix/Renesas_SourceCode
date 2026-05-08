/* V2.0 Screen UI Components */
#ifndef SCREEN_UI_H
#define SCREEN_UI_H

#include "screen.h"

/* ========== Button 组件 ========== */
/* 按键结构体 */
typedef struct {
  int8_t location[2];          // 组件中心坐标
  uint8_t frame[3];            // 组件边框参数 长度,宽度,圆角半径
  char label[32];              // 组件标签文本
  struFont_t *ascii_font;      // ASCII字体
  struFont_UTF_t *hz_font;     // UTF字体
  ST7735_Pixel_t color[3];     // 边框颜色,填充颜色,文本颜色
  uint8_t state;               // 组件状态 0x00-未按下 0xFF-按下
} struUI_Button_t;

/* ========== Tooltip 组件 ========== */
/* 文本提示框结构体 */
typedef struct {
  int8_t location[2];          // 提示框中心坐标
  uint8_t frame[2];            // 边框参数 长度,宽度
  char text[64];              // 提示文本内容
  struFont_t *ascii_font;      // ASCII字体
  struFont_UTF_t *hz_font;     // UTF字体
  ST7735_Pixel_t color[2];     // 背景颜色，文本颜色
} struUI_Tooltip_t;

/* ========== ProgressBar 组件 ========== */
/* 加载条结构体 */
typedef struct {
  int8_t location[2];          // 加载条中心坐标
  uint8_t frame[2];            // 边框参数 长度,宽度
  ST7735_Pixel_t color[2];     // 进度条边框颜色，填充颜色
  uint8_t progress;            // 进度值 0-100
} struUI_ProgressBar_t;

/* ========== Switch 组件 ========== */
/* 开关结构体 */
typedef struct {
  int8_t location[2];          // 开关中心坐标
  uint8_t width;                // 开关宽度
  uint8_t height;               // 开关高度（通常为宽度的一半）
  ST7735_Pixel_t track_color;   // 轨道颜色（关闭状态）
  ST7735_Pixel_t thumb_color;   // 滑块颜色
  bool value;                  // 当前状态 false=关 true=开
} struUI_Switch_t;

/* ========== Slider 组件 ========== */
/* 滑块结构体 */
typedef struct {
  int8_t location[2];          // 滑块中心坐标
  uint8_t width;               // 滑块宽度
  uint8_t height;              // 滑块高度
  ST7735_Pixel_t track_color;  // 轨道颜色
  ST7735_Pixel_t thumb_color;   // 滑块颜色
  ST7735_Pixel_t progress_color; // 已填充进度颜色
  int16_t min_value;           // 最小值
  int16_t max_value;           // 最大值
  int16_t current_value;       // 当前值
} struUI_Slider_t;

/* ========== ListItem 组件 ========== */
/* 列表项结构体 */
typedef struct {
  int8_t location[2];          // 列表项左上角坐标
  uint8_t width;               // 列表项宽度
  uint8_t height;              // 列表项高度
  char text[64];              // 列表项文本
  struFont_t *font;           // 字体
  ST7735_Pixel_t bg_color;     // 背景颜色
  ST7735_Pixel_t text_color;   // 文本颜色
  ST7735_Pixel_t border_color;  // 边框颜色（可为透明）
  bool selected;              // 是否被选中
  bool show_border;           // 是否显示边框
} struUI_ListItem_t;

/* ========== 函数声明 ========== */

/* Button */
SCREEN_Event_t SCREEN_DrawButton(struUI_Button_t *button);

/* Tooltip */
SCREEN_Event_t SCREEN_DrawTooltip(struUI_Tooltip_t *tooltip);

/* ProgressBar */
SCREEN_Event_t SCREEN_DrawProgressBar(struUI_ProgressBar_t *bar);

/* Switch */
SCREEN_Event_t SCREEN_DrawSwitch(struUI_Switch_t *sw);

/* Slider */
SCREEN_Event_t SCREEN_DrawSlider(struUI_Slider_t *slider);

/* ListItem */
SCREEN_Event_t SCREEN_DrawListItem(struUI_ListItem_t *item);

/* UI组件测试函数 */
SCREEN_Event_t SCREEN_DrawUITest(uint8_t index);

#endif /* SCREEN_UI_H */