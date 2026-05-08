/* V2.0 Screen UI Components */

#include "screen_ui.h"
#include <string.h>

/* ========== Button 组件 ========== */
SCREEN_Event_t SCREEN_DrawButton(struUI_Button_t *button) {
    /* 参数校验 */
    if (button == NULL) {
        return SCREEN_PARAM_ERROR;
    }

    /* 计算按钮的左上角和右下角坐标 */
    int16_t half_width = button->frame[0] / 2;
    int16_t half_height = button->frame[1] / 2;
    int16_t x0 = button->location[0] - half_width;
    int16_t x1 = button->location[0] + half_width;
    int16_t y0 = button->location[1] - half_height;
    int16_t y1 = button->location[1] + half_height;

    SCREEN_Event_t ret;

    /* 根据状态选择绘制方式 */
    if (button->state == 0x00) {
        /* 未按下状态 - 绘制空心圆角矩形 */
        ret = SCREEN_DrawRoundRectHollow(x0, x1, y0, y1, button->frame[2], button->color[0], SCREEN_Nor);
    } else {
        /* 按下状态 - 绘制实心圆角矩形 */
        ret = SCREEN_DrawRoundRectSolid(x0, x1, y0, y1, button->frame[2], button->color[1], SCREEN_Nor);
    }
    if (ret != SCREEN_OK) return ret;

    /* 绘制标签文本（水平居中，垂直居中） */
    if (strlen(button->label) > 0 && button->ascii_font != NULL) {
        uint8_t font_width = button->ascii_font->width;
        uint8_t font_height = button->ascii_font->height;
        uint16_t str_width = strlen(button->label) * font_width;
        int16_t text_x = button->location[0] - str_width / 2;
        int16_t text_y = button->location[1] - font_height / 2;
        ret = SCREEN_DrawUTFString(text_x, text_y, button->label,
                                    button->ascii_font, button->hz_font,
                                    button->color[2], SCREEN_Nor);
    }

    return ret;
}

/* ========== Tooltip 组件 ========== */
SCREEN_Event_t SCREEN_DrawTooltip(struUI_Tooltip_t *tooltip) {
    if (tooltip == NULL) {
        return SCREEN_PARAM_ERROR;
    }

    ST7735_Pixel_t bg_color = tooltip->color[0];
    ST7735_Pixel_t text_color = tooltip->color[1];

    /* 计算提示框坐标 */
    int16_t half_width = tooltip->frame[0] / 2;
    int16_t half_height = tooltip->frame[1] / 2;
    int16_t x0 = tooltip->location[0] - half_width;
    int16_t x1 = tooltip->location[0] + half_width;
    int16_t y0 = tooltip->location[1] - half_height;
    int16_t y1 = tooltip->location[1] + half_height;

    SCREEN_Event_t ret;

    /* 绘制阴影 */
    const uint8_t shadow_offset = 2;
    const ST7735_Pixel_t shadow_color = 0x4208;
    ret = SCREEN_DrawRectSolid(x0 + shadow_offset, x1 + shadow_offset,
                               y0 + shadow_offset, y1 + shadow_offset,
                               shadow_color, SCREEN_Nor);
    if (ret != SCREEN_OK) return ret;

    /* 绘制背景 */
    ret = SCREEN_DrawRectSolid(x0, x1, y0, y1, bg_color, SCREEN_Nor);
    if (ret != SCREEN_OK) return ret;

    /* 绘制文本（动态计算字体高度） */
    if (strlen(tooltip->text) > 0 && tooltip->ascii_font != NULL) {
        uint8_t font_height = tooltip->ascii_font->height;
        int16_t text_x = x0 + 4;
        int16_t text_y = y0 + (tooltip->frame[1] - font_height) / 2;
        ret = SCREEN_DrawUTFString(text_x, text_y, tooltip->text,
                                    tooltip->ascii_font, tooltip->hz_font,
                                    text_color, SCREEN_Nor);
    }

    return ret;
}

/* ========== ProgressBar 组件 ========== */
SCREEN_Event_t SCREEN_DrawProgressBar(struUI_ProgressBar_t *bar) {
    if (bar == NULL) {
        return SCREEN_PARAM_ERROR;
    }

    /* 限制进度值范围 */
    if (bar->progress > 100) bar->progress = 100;
    if (bar->progress < 0) bar->progress = 0;

    /* 计算坐标 */
    int16_t half_width = bar->frame[0] / 2;
    int16_t half_height = bar->frame[1] / 2;
    int16_t x0 = bar->location[0] - half_width;
    int16_t x1 = bar->location[0] + half_width;
    int16_t y0 = bar->location[1] - half_height;
    int16_t y1 = bar->location[1] + half_height;
    uint8_t radius = bar->frame[1] / 2;

    SCREEN_Event_t ret;

    /* 绘制边框 */
    ret = SCREEN_DrawRoundRectHollow(x0, x1, y0, y1, radius, bar->color[0], SCREEN_Nor);
    if (ret != SCREEN_OK) return ret;

    /* 绘制进度填充 */
    if (bar->progress > 0) {
        /* 计算填充区域 */
        int16_t fill_width = (bar->frame[0] - 2) * bar->progress / 100;
        if (fill_width > 0) {
            int16_t fill_x1 = x0 + 1 + fill_width;
            /* 确保填充区域在有效范围内 */
            if (fill_x1 > x0 + 1) {
                ret = SCREEN_DrawRoundRectSolid(x0 + 1, fill_x1, y0 + 1, y1 - 1,
                                                radius > 1 ? radius - 1 : 0,
                                                bar->color[1], SCREEN_Nor);
            }
        }
    }

    return ret;
}

/* ========== Switch 组件 ========== */
SCREEN_Event_t SCREEN_DrawSwitch(struUI_Switch_t *sw) {
    if (sw == NULL) {
        return SCREEN_PARAM_ERROR;
    }

    /* 计算开关轨道矩形 */
    int16_t half_width = sw->width / 2;
    int16_t half_height = sw->height / 2;
    int16_t x0 = sw->location[0] - half_width;
    int16_t x1 = sw->location[0] + half_width;
    int16_t y0 = sw->location[1] - half_height;
    int16_t y1 = sw->location[1] + half_height;

    /* 轨道圆角半径（高度的一半） */
    uint8_t radius = sw->height / 2;

    SCREEN_Event_t ret;

    /* 绘制轨道背景 */
    ST7735_Pixel_t track_color = sw->value ? 0x07E0 : 0x6E6E;  // 开=绿色，关=灰色
    ret = SCREEN_DrawRoundRectSolid(x0, x1, y0, y1, radius, track_color, SCREEN_Nor);
    if (ret != SCREEN_OK) return ret;

    /* 计算滑块位置 */
    int16_t thumb_radius = radius - 2;
    int16_t thumb_center_offset = (sw->width / 2 - radius) * (sw->value ? 1 : -1);
    int16_t thumb_x = sw->location[0] + thumb_center_offset;
    int16_t thumb_y = sw->location[1];

    /* 绘制滑块（圆形） */
    ret = SCREEN_DrawQuarArc(thumb_x, thumb_y, thumb_radius, 0x0F, sw->thumb_color, SCREEN_Nor);

    return ret;
}

/* ========== Slider 组件 ========== */
SCREEN_Event_t SCREEN_DrawSlider(struUI_Slider_t *slider) {
    if (slider == NULL) {
        return SCREEN_PARAM_ERROR;
    }

    /* 限制值在范围内 */
    if (slider->current_value < slider->min_value) slider->current_value = slider->min_value;
    if (slider->current_value > slider->max_value) slider->current_value = slider->max_value;

    /* 计算滑块轨道矩形 */
    int16_t half_width = slider->width / 2;
    int16_t half_height = slider->height / 2;
    int16_t x0 = slider->location[0] - half_width;
    int16_t x1 = slider->location[0] + half_width;
    int16_t y0 = slider->location[1] - half_height;
    int16_t y1 = slider->location[1] + half_height;

    /* 轨道圆角半径 */
    uint8_t radius = slider->height / 2;

    SCREEN_Event_t ret;

    /* 绘制轨道背景 */
    ret = SCREEN_DrawRoundRectSolid(x0, x1, y0, y1, radius, slider->track_color, SCREEN_Nor);
    if (ret != SCREEN_OK) return ret;

    /* 计算进度填充宽度 */
    int32_t range = slider->max_value - slider->min_value;
    if (range <= 0) range = 1;
    int32_t progress = slider->current_value - slider->min_value;
    uint16_t fill_width = (slider->width - 2) * progress / range;

    /* 绘制进度填充 */
    if (fill_width > 0) {
        int16_t fill_x1 = x0 + 1 + fill_width;
        if (fill_x1 > x0 + 1) {
            ret = SCREEN_DrawRoundRectSolid(x0 + 1, fill_x1, y0 + 1, y1 - 1,
                                            radius > 1 ? radius - 1 : 0,
                                            slider->progress_color, SCREEN_Nor);
            if (ret != SCREEN_OK) return ret;
        }
    }

    /* 计算并绘制滑块（居中的小矩形） */
    uint8_t thumb_width = slider->height;
    uint8_t thumb_height = slider->height - 2;
    int16_t thumb_x0 = slider->location[0] - thumb_width / 2;
    int16_t thumb_x1 = slider->location[0] + thumb_width / 2;
    int16_t thumb_y0 = slider->location[1] - thumb_height / 2;
    int16_t thumb_y1 = slider->location[1] + thumb_height / 2;
    uint8_t thumb_radius = thumb_height / 2;

    ret = SCREEN_DrawRoundRectSolid(thumb_x0, thumb_x1, thumb_y0, thumb_y1,
                                    thumb_radius, slider->thumb_color, SCREEN_Nor);

    return ret;
}

/* ========== ListItem 组件 ========== */
SCREEN_Event_t SCREEN_DrawListItem(struUI_ListItem_t *item) {
    if (item == NULL) {
        return SCREEN_PARAM_ERROR;
    }

    int16_t x0 = item->location[0];
    int16_t x1 = item->location[0] + item->width;
    int16_t y0 = item->location[1];
    int16_t y1 = item->location[1] + item->height;

    SCREEN_Event_t ret;

    /* 绘制背景 */
    ret = SCREEN_DrawRectSolid(x0, x1, y0, y1, item->bg_color, SCREEN_Nor);
    if (ret != SCREEN_OK) return ret;

    /* 可选：绘制边框 */
    if (item->show_border && item->border_color != 0) {
        ret = SCREEN_DrawRectHollow(x0, x1, y0, y1, item->border_color, SCREEN_Nor);
        if (ret != SCREEN_OK) return ret;
    }

    /* 选中文本特殊处理（反色显示） */
    SCREEN_Mode_t text_mode = item->selected ? SCREEN_Xor : SCREEN_Nor;
    ST7735_Pixel_t display_text_color = item->selected ? ~item->text_color : item->text_color;

    /* 绘制文本（垂直居中） */
    if (strlen(item->text) > 0 && item->font != NULL) {
        uint8_t font_height = item->font->height;
        int16_t text_y = item->location[1] + (item->height - font_height) / 2;
        int16_t text_x = item->location[0] + 4;  // 左边留4像素边距
        ret = SCREEN_DrawUTFString(text_x, text_y, item->text,
                                    item->font, NULL,
                                    display_text_color, text_mode);
    }

    /* 选中状态：绘制左侧选中指示条 */
    if (item->selected) {
        const uint8_t indicator_width = 3;
        ret = SCREEN_DrawRectSolid(x0, x0 + indicator_width, y0, y1,
                                    item->text_color, SCREEN_Nor);
    }

    return ret;
}

/* ========== UI组件测试函数 ========== */
SCREEN_Event_t SCREEN_DrawUITest(uint8_t index) {
    /* 静态字体声明 */
    extern const struFont_t Font_8x16_consola;
    static struUI_Button_t btn;
    static struUI_Tooltip_t tip;
    static struUI_ProgressBar_t bar;
    static struUI_Switch_t sw;
    static struUI_Slider_t slider;
    static struUI_ListItem_t item;

    SCREEN_Event_t ret;

    switch (index) {
        case 0: {
            /* Button测试 - 屏幕左上区域 */
            btn.location[0] = 40;
            btn.location[1] = 30;
            btn.frame[0] = 60;   // 宽度
            btn.frame[1] = 24;  // 高度
            btn.frame[2] = 4;    // 圆角半径
            strcpy(btn.label, "Button");
            btn.ascii_font = &Font_8x16_consola;
            btn.hz_font = &Font_UTF_16x16_YuMincho;
            btn.color[0] = 0x07E0;  // 边框绿色
            btn.color[1] = 0x03E0;  // 填充深绿
            btn.color[2] = 0x0000;  // 文本黑色
            btn.state = 0x00;
            ret = SCREEN_DrawButton(&btn);

            /* 按下状态的Button */
            btn.location[1] += 35;
            btn.state = 0xFF;
            ret = SCREEN_DrawButton(&btn);
            break;
        }

        case 1: {
            /* Tooltip测试 - 屏幕上方 */
            tip.location[0] = 64;
            tip.location[1] = 25;
            tip.frame[0] = 108;  // 宽度
            tip.frame[1] = 24;   // 高度
            strcpy(tip.text, "Tooltip Test");
            tip.ascii_font = &Font_8x16_consola;
            tip.hz_font = &Font_UTF_16x16_YuMincho;
            tip.color[0] = 0xFFFF;  // 白色背景
            tip.color[1] = 0x0000;  // 黑色文本
            ret = SCREEN_DrawTooltip(&tip);
            break;
        }

        case 2: {
            /* ProgressBar测试 - 屏幕中央 */
            bar.location[0] = 64;
            bar.location[1] = 30;
            bar.frame[0] = 100;  // 宽度
            bar.frame[1] = 16;  // 高度
            bar.color[0] = 0xFFFF;  // 边框白色
            bar.color[1] = 0x07E0;  // 填充绿色
            bar.progress = 66;
            ret = SCREEN_DrawProgressBar(&bar);

            /* 另一个进度条 */
            bar.location[1] += 30;
            bar.progress = 33;
            bar.color[1] = 0xF800;  // 填充红色
            ret = SCREEN_DrawProgressBar(&bar);
            break;
        }

        case 3: {
            /* Switch测试 - 屏幕上方 */
            sw.location[0] = 40;
            sw.location[1] = 25;
            sw.width = 50;
            sw.height = 26;
            sw.track_color = 0x6E6E;  // 关闭灰色
            sw.thumb_color = 0xFFFF;  // 滑块白色
            sw.value = false;
            ret = SCREEN_DrawSwitch(&sw);

            /* 开状态的Switch */
            sw.location[0] = 100;
            sw.value = true;
            sw.track_color = 0x07E0;  // 开启绿色
            ret = SCREEN_DrawSwitch(&sw);
            break;
        }

        case 4: {
            /* Slider测试 - 屏幕中央 */
            slider.location[0] = 64;
            slider.location[1] = 25;
            slider.width = 100;
            slider.height = 20;
            slider.track_color = 0x6E6E;   // 轨道灰色
            slider.thumb_color = 0xFFFF;   // 滑块白色
            slider.progress_color = 0x07E0; // 进度绿色
            slider.min_value = 0;
            slider.max_value = 100;
            slider.current_value = 60;
            ret = SCREEN_DrawSlider(&slider);

            /* 另一个Slider */
            slider.location[1] += 35;
            slider.current_value = 30;
            slider.progress_color = 0xF800;  // 进度红色
            ret = SCREEN_DrawSlider(&slider);
            break;
        }

        case 5: {
            /* ListItem测试 - 屏幕左侧 */
            item.location[0] = 0;
            item.location[1] = 10;
            item.width = 120;
            item.height = 22;
            strcpy(item.text, "List Item 1");
            item.font = &Font_8x16_consola;
            item.bg_color = 0x0000;       // 黑色背景
            item.text_color = 0xFFFF;     // 白色文本
            item.border_color = 0x6E6E;   // 灰色边框
            item.selected = false;
            item.show_border = true;
            ret = SCREEN_DrawListItem(&item);

            /* 选中的ListItem */
            item.location[1] += 25;
            strcpy(item.text, "List Item 2");
            item.selected = true;
            ret = SCREEN_DrawListItem(&item);

            /* 第三个ListItem */
            item.location[1] += 25;
            strcpy(item.text, "List Item 3");
            item.selected = false;
            ret = SCREEN_DrawListItem(&item);
            break;
        }

        default:
            ret = SCREEN_PARAM_ERROR;
            break;
    }

    return ret;
}