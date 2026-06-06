/* V2.0 Screen UI Components */
#ifndef SCREEN_UI_H
#define SCREEN_UI_H

#include "screen.h"

/* ========== Button 组件 ========== */
typedef struct {
    int8_t location[2];
    uint8_t frame[3];
    char label[32];
    const struFont_t *ascii_font;
    const struFont_UTF_t *hz_font;
    SCREEN_Pixel_t color[3];
    uint8_t state;
} UI_Button_t;

/* ========== Tooltip 组件 ========== */
typedef struct {
    int8_t location[2];
    uint8_t frame[2];
    char text[64];
    const struFont_t *ascii_font;
    const struFont_UTF_t *hz_font;
    SCREEN_Pixel_t color[2];
} UI_Tooltip_t;

/* ========== ProgressBar 组件 ========== */
typedef struct {
    int8_t location[2];
    uint8_t frame[2];
    SCREEN_Pixel_t color[2];
    uint8_t progress;
} UI_ProgressBar_t;

/* ========== Switch 组件 ========== */
typedef struct {
    int8_t location[2];
    uint8_t width;
    uint8_t height;
    SCREEN_Pixel_t track_color;
    SCREEN_Pixel_t thumb_color;
    bool value;
} UI_Switch_t;

/* ========== Slider 组件 ========== */
typedef struct {
    int8_t location[2];
    uint8_t width;
    uint8_t height;
    SCREEN_Pixel_t track_color;
    SCREEN_Pixel_t thumb_color;
    SCREEN_Pixel_t progress_color;
    int16_t min_value;
    int16_t max_value;
    int16_t current_value;
} UI_Slider_t;

/* ========== ListItem 组件 ========== */
typedef struct {
    int8_t location[2];
    uint8_t width;
    uint8_t height;
    char text[64];
    const struFont_t *font;
    SCREEN_Pixel_t bg_color;
    SCREEN_Pixel_t text_color;
    SCREEN_Pixel_t border_color;
    bool selected;
    bool show_border;
} UI_ListItem_t;

/* ========== 函数声明 ========== */
SCREEN_Event_t UI_DrawButton(UI_Button_t *button);
SCREEN_Event_t UI_DrawTooltip(UI_Tooltip_t *tooltip);
SCREEN_Event_t UI_DrawProgressBar(UI_ProgressBar_t *bar);
SCREEN_Event_t UI_DrawSwitch(UI_Switch_t *sw);
SCREEN_Event_t UI_DrawSlider(UI_Slider_t *slider);
SCREEN_Event_t UI_DrawListItem(UI_ListItem_t *item);
SCREEN_Event_t UI_DrawTest(uint8_t index);

#endif /* SCREEN_UI_H */