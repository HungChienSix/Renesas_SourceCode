#include "screen_ui.h"
#include <string.h>

SCREEN_Event_t SCREEN_DrawButton(struUI_Button_t *button, SCREEN_Pixel_t Pixel_Set, SCREEN_Mode_t type){
    // 参数校验
    if(button == NULL){
        return SCREEN_PARAM_ERROR;
    }

    // 计算按钮的左上角和右下角坐标
    // location[0] 是中心x坐标，location[1] 是中心y坐标
    // frame[0] 是长度，frame[1] 是宽度，frame[2] 是圆角半径
    int16_t half_width = button->frame[0] / 2;
    int16_t half_height = button->frame[1] / 2;

    int16_t x0 = button->location[0] - half_width;
    int16_t x1 = button->location[0] + half_width;
    int16_t y0 = button->location[1] - half_height;
    int16_t y1 = button->location[1] + half_height;

    SCREEN_Event_t ret;

    // 根据状态选择绘制方式
    if(button->state == 0x00){
        // 未按下状态 - 绘制空心圆角矩形
        ret = SCREEN_DrawRoundRectHollow(x0, x1, y0, y1, button->frame[2], Pixel_Set, type);
    }else{
        // 按下状态 - 绘制实心圆角矩形
        ret = SCREEN_DrawRoundRectSolid(x0, x1, y0, y1, button->frame[2], Pixel_Set, type);
    }

    if(ret != SCREEN_OK){
        return ret;
    }

    // 绘制标签文本
    if(strlen(button->label) > 0){
        // 计算文本起始位置，使其居中
        int16_t text_x = x0 + 2; // 左边留2像素边距
        int16_t text_y = button->location[1];

        // 判断是纯ASCII还是包含UTF-8中文
        uint8_t has_utf8 = 0;
        for(uint8_t i = 0; button->label[i] != '\0'; i++){
            if((button->label[i] & 0x80) != 0){
                has_utf8 = 1;
                break;
            }
        }

        if(has_utf8 && button->hz_font != NULL){
            // 包含中文，使用UTF-8绘制函数
            ret = SCREEN_DrawUTF8String(text_x, text_y, button->label,
                                        button->ascii_font, button->hz_font,
                                        Pixel_Set, type);
        }else if(button->ascii_font != NULL){
            // 纯ASCII，使用ASCII绘制函数
            // 需要手动调整y使文本垂直居中
            text_y = y0 + (button->frame[1] - button->ascii_font->height) / 2;
            ret = SCREEN_DrawString(text_x, text_y, button->label,
                                   button->ascii_font, Pixel_Set, type);
        }
    }

    return ret;
}

SCREEN_Event_t SCREEN_DrawTooltip(struUI_Tooltip_t *tooltip, SCREEN_Pixel_t Pixel_Set, SCREEN_Pixel_t bg_color, SCREEN_Mode_t type){
    // 参数校验
    if(tooltip == NULL){
        return SCREEN_PARAM_ERROR;
    }

    // 阴影偏移量
    const uint8_t shadow_offset = 2;
    // 阴影颜色（使用暗灰色）
    const SCREEN_Pixel_t shadow_color = 0x4208;  // RGB565暗灰色

    // 计算提示框的左上角和右下角坐标
    int16_t half_width = tooltip->frame[0] / 2;
    int16_t half_height = tooltip->frame[1] / 2;

    int16_t x0 = tooltip->location[0] - half_width;
    int16_t x1 = tooltip->location[0] + half_width;
    int16_t y0 = tooltip->location[1] - half_height;
    int16_t y1 = tooltip->location[1] + half_height;

    SCREEN_Event_t ret;

    // 第一步：绘制阴影（右下方偏移）
    ret = SCREEN_DrawRectSolid(x0 + shadow_offset, x1 + shadow_offset,
                               y0 + shadow_offset, y1 + shadow_offset,
                               shadow_color, SCREEN_Nor);
    if(ret != SCREEN_OK){
        return ret;
    }

    // 第二步：绘制无边框实心背景框
    ret = SCREEN_DrawRectSolid(x0, x1, y0, y1, bg_color, type);

    if(ret != SCREEN_OK){
        return ret;
    }

    // 第三步：绘制文本内容
    if(strlen(tooltip->text) > 0){
        // 计算文本起始位置
        int16_t text_x = x0 + 4; // 左边留4像素边距
        int16_t text_y;

        // 判断是纯ASCII还是包含UTF-8中文
        uint8_t has_utf8 = 0;
        for(uint8_t i = 0; tooltip->text[i] != '\0'; i++){
            if((tooltip->text[i] & 0x80) != 0){
                has_utf8 = 1;
                break;
            }
        }

        if(has_utf8 && tooltip->hz_font != NULL){
            // 包含中文，使用UTF-8绘制函数
            // 垂直居中（假设UTF字体高度约为12-16）
            text_y = y0 + (tooltip->frame[1] - 12) / 2;
            ret = SCREEN_DrawUTF8String(text_x, text_y, tooltip->text,
                                        tooltip->ascii_font, tooltip->hz_font,
                                        Pixel_Set, type);
        }else if(tooltip->ascii_font != NULL){
            // 纯ASCII，使用ASCII绘制函数
            // 垂直居中
            text_y = y0 + (tooltip->frame[1] - tooltip->ascii_font->height) / 2;
            ret = SCREEN_DrawString(text_x, text_y, tooltip->text,
                                   tooltip->ascii_font, Pixel_Set, type);
        }
    }

    return ret;
}

SCREEN_Event_t SCREEN_DrawProgressBar(struUI_ProgressBar_t *bar, SCREEN_Pixel_t Pixel_Set, SCREEN_Pixel_t fill_color, SCREEN_Mode_t type){
    // 参数校验
    if(bar == NULL){
        return SCREEN_PARAM_ERROR;
    }

    // 限制进度值范围 0-100
    if(bar->progress > 100){
        bar->progress = 100;
    }

    // 计算加载条的左上角和右下角坐标
    int16_t half_width = bar->frame[0] / 2;
    int16_t half_height = bar->frame[1] / 2;

    int16_t x0 = bar->location[0] - half_width;
    int16_t x1 = bar->location[0] + half_width;
    int16_t y0 = bar->location[1] - half_height;
    int16_t y1 = bar->location[1] + half_height;

    // 圆角半径 = 高度的一半，形成两端半圆
    uint8_t radius = bar->frame[1] / 2;

    SCREEN_Event_t ret;

    // 第一步：绘制边框（圆角矩形空心）
    ret = SCREEN_DrawRoundRectHollow(x0, x1, y0, y1, radius, Pixel_Set, type);
    if(ret != SCREEN_OK){
        return ret;
    }

    // 第二步：绘制进度填充（圆角矩形实心）
    if(bar->progress > 0){
        // 计算填充的右边界（留出1像素边距避免覆盖边框）
        int16_t fill_width = (bar->frame[0] - 2) * bar->progress / 100;
        int16_t fill_x1 = x0 + 1 + fill_width;

        // 填充区域使用圆角矩形
        ret = SCREEN_DrawRoundRectSolid(x0 + 1, fill_x1, y0 + 1, y1 - 1, radius - 1, fill_color, type);
        if(ret != SCREEN_OK){
            return ret;
        }
    }

    return SCREEN_OK;
}