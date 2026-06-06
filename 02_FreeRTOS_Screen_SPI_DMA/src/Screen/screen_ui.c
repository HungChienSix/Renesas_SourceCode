/* V2.0 Screen UI Components */

#include "screen_ui.h"
#include <string.h>

/* ========== Button ========== */
SCREEN_Event_t UI_DrawButton(UI_Button_t *btn) {
    if (btn == NULL)
        return SCREEN_PARAM_ERROR;

    int16_t half_w = btn->frame[0] / 2;
    int16_t half_h = btn->frame[1] / 2;
    int16_t x0 = btn->location[0] - half_w;
    int16_t x1 = btn->location[0] + half_w;
    int16_t y0 = btn->location[1] - half_h;
    int16_t y1 = btn->location[1] + half_h;

    SCREEN_Event_t ret;
    if (btn->state == 0x00) {
        ret = SCREEN_DrawRoundRectHollow(x0, x1, y0, y1, btn->frame[2], btn->color[0], SCREEN_Nor);
    } else {
        ret = SCREEN_DrawRoundRect(x0, x1, y0, y1, btn->frame[2], btn->color[1], SCREEN_Nor);
    }
    if (ret != SCREEN_OK) return ret;

    if (strlen(btn->label) > 0 && btn->ascii_font != NULL) {
        uint8_t fw = btn->ascii_font->width;
        uint8_t fh = btn->ascii_font->height;
        uint16_t str_w = strlen(btn->label) * fw;
        int16_t tx = btn->location[0] - str_w / 2;
        int16_t ty = btn->location[1] - fh / 2;
        ret = SCREEN_DrawUTFString(tx, ty, btn->label, btn->ascii_font, btn->hz_font, btn->color[2], SCREEN_Nor);
    }
    return ret;
}

/* ========== Tooltip ========== */
SCREEN_Event_t UI_DrawTooltip(UI_Tooltip_t *tip) {
    if (tip == NULL)
        return SCREEN_PARAM_ERROR;

    int16_t half_w = tip->frame[0] / 2;
    int16_t half_h = tip->frame[1] / 2;
    int16_t x0 = tip->location[0] - half_w;
    int16_t x1 = tip->location[0] + half_w;
    int16_t y0 = tip->location[1] - half_h;
    int16_t y1 = tip->location[1] + half_h;

    SCREEN_Event_t ret;
    ret = SCREEN_DrawRect(x0 + 2, x1 + 2, y0 + 2, y1 + 2, 0x4208, SCREEN_Nor);
    if (ret != SCREEN_OK) return ret;

    ret = SCREEN_DrawRect(x0, x1, y0, y1, tip->color[0], SCREEN_Nor);
    if (ret != SCREEN_OK) return ret;

    if (strlen(tip->text) > 0 && tip->ascii_font != NULL) {
        uint8_t fh = tip->ascii_font->height;
        int16_t tx = x0 + 4;
        int16_t ty = y0 + (tip->frame[1] - fh) / 2;
        ret = SCREEN_DrawUTFString(tx, ty, tip->text, tip->ascii_font, tip->hz_font, tip->color[1], SCREEN_Nor);
    }
    return ret;
}

/* ========== ProgressBar ========== */
SCREEN_Event_t UI_DrawProgressBar(UI_ProgressBar_t *bar) {
    if (bar == NULL)
        return SCREEN_PARAM_ERROR;

    if (bar->progress > 100) bar->progress = 100;
    /* F-34: progress 是 uint8_t, 原 < 0 检查恒为假, 已删除 */

    int16_t half_w = bar->frame[0] / 2;
    int16_t half_h = bar->frame[1] / 2;
    int16_t x0 = bar->location[0] - half_w;
    int16_t x1 = bar->location[0] + half_w;
    int16_t y0 = bar->location[1] - half_h;
    int16_t y1 = bar->location[1] + half_h;
    uint8_t radius = bar->frame[1] / 2;

    SCREEN_Event_t ret;
    ret = SCREEN_DrawRoundRectHollow(x0, x1, y0, y1, radius, bar->color[0], SCREEN_Nor);
    if (ret != SCREEN_OK) return ret;

    if (bar->progress > 0) {
        int16_t fill_w = (bar->frame[0] - 2) * bar->progress / 100;
        if (fill_w > 0) {
            int16_t fx1 = x0 + 1 + fill_w;
            if (fx1 > x0 + 1)
                ret = SCREEN_DrawRoundRect(x0 + 1, fx1, y0 + 1, y1 - 1, radius > 1 ? radius - 1 : 0, bar->color[1], SCREEN_Nor);
        }
    }
    return ret;
}

/* ========== Switch ========== */
SCREEN_Event_t UI_DrawSwitch(UI_Switch_t *sw) {
    if (sw == NULL)
        return SCREEN_PARAM_ERROR;

    int16_t half_w = sw->width / 2;
    int16_t half_h = sw->height / 2;
    int16_t x0 = sw->location[0] - half_w;
    int16_t x1 = sw->location[0] + half_w;
    int16_t y0 = sw->location[1] - half_h;
    int16_t y1 = sw->location[1] + half_h;
    uint8_t radius = sw->height / 2;
    /* F-35: 高度过小时 thumb_radius=radius-2 下溢 (转 uint16_t 后变成 65534)
     * radius<2 还会让 track 圆角无意义, 一并拒绝 */
    if (radius < 2) return SCREEN_PARAM_ERROR;

    SCREEN_Event_t ret;
    SCREEN_Pixel_t track_color = sw->value ? 0x07E0 : 0x6E6E;
    ret = SCREEN_DrawRoundRect(x0, x1, y0, y1, radius, track_color, SCREEN_Nor);
    if (ret != SCREEN_OK) return ret;

    int16_t thumb_radius = radius - 2;
    int16_t offset = (sw->width / 2 - radius) * (sw->value ? 1 : -1);
    int16_t thumb_x = sw->location[0] + offset;
    int16_t thumb_y = sw->location[1];

    ret = SCREEN_DrawArc(thumb_x, thumb_y, thumb_radius, 0x0F, sw->thumb_color, SCREEN_Nor);
    return ret;
}

/* ========== Slider ========== */
SCREEN_Event_t UI_DrawSlider(UI_Slider_t *slider) {
    if (slider == NULL)
        return SCREEN_PARAM_ERROR;

    /* F-33: 拒绝 min>=max, 避免 (max-min)<=0 后续 int16_t 溢出成 huge fill_w */
    if (slider->max_value <= slider->min_value) return SCREEN_PARAM_ERROR;

    if (slider->current_value < slider->min_value) slider->current_value = slider->min_value;
    if (slider->current_value > slider->max_value) slider->current_value = slider->max_value;

    int16_t half_w = slider->width / 2;
    int16_t half_h = slider->height / 2;
    int16_t x0 = slider->location[0] - half_w;
    int16_t x1 = slider->location[0] + half_w;
    int16_t y0 = slider->location[1] - half_h;
    int16_t y1 = slider->location[1] + half_h;
    uint8_t radius = slider->height / 2;

    SCREEN_Event_t ret;
    ret = SCREEN_DrawRoundRect(x0, x1, y0, y1, radius, slider->track_color, SCREEN_Nor);
    if (ret != SCREEN_OK) return ret;

    int32_t range = slider->max_value - slider->min_value;
    if (range <= 0) range = 1;
    int32_t progress = slider->current_value - slider->min_value;
    uint16_t fill_w = (slider->width - 2) * progress / range;

    if (fill_w > 0) {
        int16_t fx1 = x0 + 1 + fill_w;
        if (fx1 > x0 + 1)
            ret = SCREEN_DrawRoundRect(x0 + 1, fx1, y0 + 1, y1 - 1, radius > 1 ? radius - 1 : 0, slider->progress_color, SCREEN_Nor);
        if (ret != SCREEN_OK) return ret;
    }

    uint8_t thumb_w = slider->height;
    uint8_t thumb_h = slider->height - 2;
    int16_t tx0 = slider->location[0] - thumb_w / 2;
    int16_t tx1 = slider->location[0] + thumb_w / 2;
    int16_t ty0 = slider->location[1] - thumb_h / 2;
    int16_t ty1 = slider->location[1] + thumb_h / 2;
    uint8_t thumb_radius = thumb_h / 2;

    ret = SCREEN_DrawRoundRect(tx0, tx1, ty0, ty1, thumb_radius, slider->thumb_color, SCREEN_Nor);
    return ret;
}

/* ========== ListItem ========== */
SCREEN_Event_t UI_DrawListItem(UI_ListItem_t *item) {
    if (item == NULL)
        return SCREEN_PARAM_ERROR;

    int16_t x0 = item->location[0];
    int16_t x1 = item->location[0] + item->width;
    int16_t y0 = item->location[1];
    int16_t y1 = item->location[1] + item->height;

    SCREEN_Event_t ret;
    ret = SCREEN_DrawRect(x0, x1, y0, y1, item->bg_color, SCREEN_Nor);
    if (ret != SCREEN_OK) return ret;

    if (item->show_border && item->border_color != 0) {
        ret = SCREEN_DrawRectHollow(x0, x1, y0, y1, item->border_color, SCREEN_Nor);
        if (ret != SCREEN_OK) return ret;
    }

    SCREEN_Mode_t text_mode = item->selected ? SCREEN_Xor : SCREEN_Nor;
    SCREEN_Pixel_t text_color = item->selected ? ~item->text_color : item->text_color;

    if (strlen(item->text) > 0 && item->font != NULL) {
        uint8_t fh = item->font->height;
        int16_t ty = item->location[1] + (item->height - fh) / 2;
        int16_t tx = item->location[0] + 4;
        ret = SCREEN_DrawUTFString(tx, ty, item->text, item->font, NULL, text_color, text_mode);
    }

    if (item->selected) {
        ret = SCREEN_DrawRect(x0, x0 + 3, y0, y1, item->text_color, SCREEN_Nor);
    }

    return ret;
}

/* ========== UI Test ========== */
SCREEN_Event_t UI_DrawTest(uint8_t index) {
    extern const struFont_t Font_8x16_consola;
    extern const struFont_UTF_t Font_UTF_16x16_YuMincho;
    static UI_Button_t btn;
    static UI_Tooltip_t tip;
    static UI_ProgressBar_t bar;
    static UI_Switch_t sw;
    static UI_Slider_t slider;
    static UI_ListItem_t item;

    SCREEN_Event_t ret;

    switch (index) {
        case 0:
            btn.location[0] = 40;
            btn.location[1] = 30;
            btn.frame[0] = 60;
            btn.frame[1] = 24;
            btn.frame[2] = 4;
            strcpy(btn.label, "Button");
            btn.ascii_font = &Font_8x16_consola;
            btn.hz_font = &Font_UTF_16x16_YuMincho;
            btn.color[0] = 0x07E0;
            btn.color[1] = 0x03E0;
            btn.color[2] = 0x0000;
            btn.state = 0x00;
            ret = UI_DrawButton(&btn);

            btn.location[1] += 35;
            btn.state = 0xFF;
            ret = UI_DrawButton(&btn);
            break;

        case 1:
            tip.location[0] = 64;
            tip.location[1] = 25;
            tip.frame[0] = 108;
            tip.frame[1] = 24;
            strcpy(tip.text, "Tooltip Test");
            tip.ascii_font = &Font_8x16_consola;
            tip.hz_font = &Font_UTF_16x16_YuMincho;
            tip.color[0] = 0xFFFF;
            tip.color[1] = 0x0000;
            ret = UI_DrawTooltip(&tip);
            break;

        case 2:
            bar.location[0] = 64;
            bar.location[1] = 30;
            bar.frame[0] = 100;
            bar.frame[1] = 16;
            bar.color[0] = 0xFFFF;
            bar.color[1] = 0x07E0;
            bar.progress = 66;
            ret = UI_DrawProgressBar(&bar);

            bar.location[1] += 30;
            bar.progress = 33;
            bar.color[1] = 0xF800;
            ret = UI_DrawProgressBar(&bar);
            break;

        case 3:
            sw.location[0] = 40;
            sw.location[1] = 25;
            sw.width = 50;
            sw.height = 26;
            sw.track_color = 0x6E6E;
            sw.thumb_color = 0xFFFF;
            sw.value = false;
            ret = UI_DrawSwitch(&sw);

            sw.location[0] = 100;
            sw.value = true;
            sw.track_color = 0x07E0;
            ret = UI_DrawSwitch(&sw);
            break;

        case 4:
            slider.location[0] = 64;
            slider.location[1] = 25;
            slider.width = 100;
            slider.height = 20;
            slider.track_color = 0x6E6E;
            slider.thumb_color = 0xFFFF;
            slider.progress_color = 0x07E0;
            slider.min_value = 0;
            slider.max_value = 100;
            slider.current_value = 60;
            ret = UI_DrawSlider(&slider);

            slider.location[1] += 35;
            slider.current_value = 30;
            slider.progress_color = 0xF800;
            ret = UI_DrawSlider(&slider);
            break;

        case 5:
            item.location[0] = 0;
            item.location[1] = 10;
            item.width = 120;
            item.height = 22;
            strcpy(item.text, "List Item 1");
            item.font = &Font_8x16_consola;
            item.bg_color = 0x0000;
            item.text_color = 0xFFFF;
            item.border_color = 0x6E6E;
            item.selected = false;
            item.show_border = true;
            ret = UI_DrawListItem(&item);

            item.location[1] += 25;
            strcpy(item.text, "List Item 2");
            item.selected = true;
            ret = UI_DrawListItem(&item);

            item.location[1] += 25;
            strcpy(item.text, "List Item 3");
            item.selected = false;
            ret = UI_DrawListItem(&item);
            break;

        default:
            ret = SCREEN_PARAM_ERROR;
            break;
    }

    return ret;
}