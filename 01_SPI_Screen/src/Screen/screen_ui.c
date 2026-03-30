#include "screen_ui.h"
#include <string.h>

SCREEN_Event_t SCREEN_DrawButton(struUI_Button_t *button){
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

    // 根据状态选择绘制方式，使用结构体中的颜色
    // color[0] = 边框颜色, color[1] = 填充颜色, color[2] = 文本颜色
    if(button->state == 0x00){
        // 未按下状态 - 绘制空心圆角矩形
        ret = SCREEN_DrawRoundRectHollow(x0, x1, y0, y1, button->frame[2], button->color[0], SCREEN_Nor);
    }else{
        // 按下状态 - 绘制实心圆角矩形
        ret = SCREEN_DrawRoundRectSolid(x0, x1, y0, y1, button->frame[2], button->color[1], SCREEN_Nor);
    }

    if(ret != SCREEN_OK){
        return ret;
    }

    // 绘制标签文本
    if(strlen(button->label) > 0){
        // 计算文本起始位置，使其居中
        int16_t text_x = x0 + 2; // 左边留2像素边距
        int16_t text_y = button->location[1];

        // 包含中文，使用UTF-8绘制函数
        ret = SCREEN_DrawUTFString(text_x, text_y, button->label,
                                    button->ascii_font, button->hz_font,
                                    button->color[2], SCREEN_Nor);
        
    }

    return ret;
}

SCREEN_Event_t SCREEN_DrawTooltip(struUI_Tooltip_t *tooltip){
    // 参数校验
    if(tooltip == NULL){
        return SCREEN_PARAM_ERROR;
    }

    // 使用结构体中的颜色: color[0] = 背景颜色, color[1] = 文本颜色
    SCREEN_Pixel_t bg_color = tooltip->color[0];
    SCREEN_Pixel_t text_color = tooltip->color[1];

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
    ret = SCREEN_DrawRectSolid(x0, x1, y0, y1, bg_color, SCREEN_Nor);

    if(ret != SCREEN_OK){
        return ret;
    }

    // 第三步：绘制文本内容
    if(strlen(tooltip->text) > 0){
        // 计算文本起始位置
        int16_t text_x = x0 + 4; // 左边留4像素边距
        int16_t text_y;

        // 包含中文，使用UTF-8绘制函数
        // 垂直居中（假设UTF字体高度约为12-16）
        text_y = y0 + (tooltip->frame[1] - 12) / 2;
        ret = SCREEN_DrawUTFString(text_x, text_y, tooltip->text,
                                    tooltip->ascii_font, tooltip->hz_font,
                                    text_color, SCREEN_Nor);

    }

    return ret;
}

SCREEN_Event_t SCREEN_DrawProgressBar(struUI_ProgressBar_t *bar){
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

    // 第一步：绘制边框（圆角矩形空心）- 使用白色
    ret = SCREEN_DrawRoundRectHollow(x0, x1, y0, y1, radius, bar->color[0], SCREEN_Nor);
    if(ret != SCREEN_OK){
        return ret;
    }

    // 第二步：绘制进度填充（圆角矩形实心）
    if(bar->progress > 0){
        // 计算填充的右边界（留出1像素边距避免覆盖边框）
        int16_t fill_width = (bar->frame[0] - 2) * bar->progress / 100;
        int16_t fill_x1 = x0 + 1 + fill_width;

        // 填充区域使用圆角矩形
        ret = SCREEN_DrawRoundRectSolid(x0 + 1, fill_x1, y0 + 1, y1 - 1, radius - 1, bar->color[1], SCREEN_Nor);
        if(ret != SCREEN_OK){
            return ret;
        }
    }

    return SCREEN_OK;
}


void SCREEN_Test(uint8_t screen_num) {
    uint32_t Time = 0x00;

    if(screen_num == 0) {
        // === 界面1：基础图形 + UI组件测试 ===
        // 1. FillScreen: 黑色背景
        SCREEN_FillScreen(SCREEN_BLACK);

        // 2. DrawString: 标题
        SCREEN_DrawString(30, 2, "Screen 1",
            &Font_8x12_consolas, SCREEN_YELLOW, SCREEN_Nor);

        // 3. DrawLine: 灰色分隔线
        SCREEN_DrawLine(2, 125, 14, 14,
            SCREEN_RED, SCREEN_Nor);

        // 4. DrawPixel: 像素点 (红绿黄)
        SCREEN_DrawPixel(3,  19, SCREEN_RED,   SCREEN_Nor);
        SCREEN_DrawPixel(6,  19, SCREEN_GREEN, SCREEN_Nor);
        SCREEN_DrawPixel(9,  19, SCREEN_YELLOW,  SCREEN_Nor);

        // 5. DrawChar: 单字符
        SCREEN_DrawChar(16, 17, 'A',
            &Font_8x12_consolas, SCREEN_GREEN, SCREEN_Nor);

        // 6. DrawRectSolid: 实心矩形
        SCREEN_DrawRectSolid(30, 60, 18, 38,
            SCREEN_RED, SCREEN_Nor);

        // 7. DrawRectHollow: 空心矩形
        SCREEN_DrawRectHollow(65, 95, 18, 38,
            SCREEN_GREEN, SCREEN_Nor);

        // 8. DrawQuarArc: 四段圆弧组成圆形
        SCREEN_DrawQuarArc(112, 28, 10,
            SCREEN_Quarter1 | SCREEN_Quarter2
            | SCREEN_Quarter3 | SCREEN_Quarter4,
            SCREEN_YELLOW, SCREEN_Nor);

        // 9. DrawHorLine: 水平线
        DrawHorLine(3, 55, 44, SCREEN_RED, SCREEN_Nor);

        // 10. DrawVerLine: 垂直线
        DrawVerLine(60, 44, 68, SCREEN_GREEN, SCREEN_Nor);

        // 11. DrawLine: 对角线 (正常模式)
        SCREEN_DrawLine(66, 100, 48, 68,
            SCREEN_YELLOW, SCREEN_Nor);

        // 12. DrawLine: 对角线 (XOR模式, 与上一条叠加变黑)
        SCREEN_DrawLine(68, 98, 50, 66,
            SCREEN_YELLOW, SCREEN_Xor);

        // 13. DrawButton: 按钮组件
        struUI_Button_t btn = {
            .location = {25, 88},
            .frame = {30, 14, 3},
            .label = "OK",
            .ascii_font = (struFont_t *)&Font_8x12_consolas,
            .hz_font = NULL,
            .color = {SCREEN_GREEN, SCREEN_BLACK, SCREEN_YELLOW},
            .state = 0x00
        };
        SCREEN_DrawButton(&btn);

        // 14. DrawProgressBar: 进度条
        struUI_ProgressBar_t bar = {
            .location = {90, 88},
            .frame = {40, 8},
            .color = {SCREEN_RED, SCREEN_GREEN},
            .progress = 75
        };
        SCREEN_DrawProgressBar(&bar);

    } else {
        // === 界面2：进阶图形 + UTF + 图片测试 ===
        // printf(">> S2 start\r\n");

        // 1. FillScreen: 深蓝色背景
        SCREEN_FillScreen(SCREEN_BLACK);
        // printf(">> S2-1 FillScreen OK\r\n");

        // 2. DrawString: 标题
        SCREEN_DrawString(30, 2, "Screen 2",
            &Font_8x12_consolas, SCREEN_YELLOW, SCREEN_Nor);
        // printf(">> S2-2 DrawString OK\r\n");

        // 3. DrawLine: 灰色分隔线
        SCREEN_DrawLine(2, 125, 14, 14,
            SCREEN_RED, SCREEN_Nor);
        // printf(">> S2-3 DrawLine OK\r\n");

        // 4. DrawRoundRectSolid: 实心圆角矩形
        SCREEN_DrawRoundRectSolid(3, 55, 18, 42, 6,
            SCREEN_RED, SCREEN_Nor);
        // printf(">> S2-4 RoundRectSolid OK\r\n");

        // 5. DrawRoundRectHollow: 空心圆角矩形
        SCREEN_DrawRoundRectHollow(68, 122, 18, 42, 6,
            SCREEN_GREEN, SCREEN_Nor);
        // printf(">> S2-5 RoundRectHollow OK\r\n");

        // 6. DrawQuarSector: 扇形-右半圆
        SCREEN_DrawQuarSector(30, 59, 14,
            SCREEN_Quarter1 | SCREEN_Quarter4,
            SCREEN_YELLOW, SCREEN_Nor);
        // printf(">> S2-6 Sector1 OK\r\n");

        // 7. DrawQuarSector: 扇形-左半圆
        SCREEN_DrawQuarSector(100, 59, 14,
            SCREEN_Quarter2 | SCREEN_Quarter3,
            SCREEN_GREEN, SCREEN_Nor);
        // printf(">> S2-7 Sector2 OK\r\n");

        // 8. DrawUTFChar: 单个中文字符
        SCREEN_DrawUTFChar(55, 48, "\xe6\xb5\x8b",
            &Font_UTF_16x12_YuMincho, SCREEN_YELLOW, SCREEN_Nor);
        // printf(">> S2-8 UTFChar OK\r\n");

        // 9. DrawImage: 单色图片 (苹果 32x32)
        SCREEN_DrawImage(5, 75, 32, 32,
            gImage_apple, SCREEN_GREEN, SCREEN_Nor);
        // printf(">> S2-9 DrawImage OK\r\n");

        // 10. DrawRGBImage: RGB图片 (32x32)
        SCREEN_DrawRGBImage(42, 75, 32, 32,
            gImage_RGB_163music);
        // printf(">> S2-10 RGBImage OK\r\n");

        // 12. DrawTooltip: 提示框
        struUI_Tooltip_t tip = {
            .location = {95, 118},
            .frame = {48, 12},
            .text = "Pass!",
            .ascii_font = (struFont_t *)&Font_8x12_consolas,
            .hz_font = NULL,
            .color = {SCREEN_RED, SCREEN_YELLOW}
        };
        SCREEN_DrawTooltip(&tip);
        // printf(">> S2-12 Tooltip OK\r\n");

        SCREEN_DrawUTFString(5, 110, "清明",
            &Font_8x12_consolas, &Font_UTF_16x12_YuMincho,
            SCREEN_YELLOW, SCREEN_Nor);
    }

    Time = SCREEN_RefreshScreen();
    R_BSP_SoftwareDelay(1000U, BSP_DELAY_UNITS_MILLISECONDS);

    printf("[界面%d] Refresh: %ldms\r\n", screen_num == 0 ? 2 : 1, Time);
}

