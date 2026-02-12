#include "user_screen.h"
#include <stdio.h>
#include "string.h"
#include "../KEY/key.h"

struINPUT_t struINPUT[2] = {0};

// 当前页面状态: 0=欢迎页, 1=页面A, 2=页面B
static uint8_t current_page = 0;

/**
 * @brief Page0 - 欢迎页面
 */
void Page0_Welcome(void)
{
    SCREEN_FillScreen(SCREEN_BLACK);

    // 显示欢迎文字
    SCREEN_DrawUTF8String(20, 40, "欢迎", &Font_8x12_consolas, &Font_UTF_16x16_YuMincho, SCREEN_WHITE, SCREEN_Nor);
    SCREEN_DrawString(30, 70, "Welcome", &Font_8x12_consolas, SCREEN_CYAN, SCREEN_Nor);

    // 提示信息
    SCREEN_DrawString(10, 100, "Press A or B", &Font_8x12_consolas, SCREEN_YELLOW, SCREEN_Nor);

    SCREEN_RefreshScreen();
}

/**
 * @brief Page1 - 按钮A页面
 */
void Page1_ButtonA(void)
{
    SCREEN_FillScreen(SCREEN_BLACK);

    // 标题
    SCREEN_DrawString(45, 20, "Page 1", &Font_8x12_consolas, SCREEN_GREEN, SCREEN_Nor);

    // 按钮 "A"
    struUI_Button_t buttonA = {
        .location = {64, 70},    // 中心坐标
        .frame = {40, 30, 5},   // 长度40, 宽度30, 圆角5
        .label = "A",
        .ascii_font = &Font_8x16_consolas,
        .hz_font = NULL,
        .state = 0x00
    };
    SCREEN_DrawButton(&buttonA, SCREEN_BLUE, SCREEN_Nor);

    // 提示返回
    SCREEN_DrawString(10, 110, "Press B back", &Font_8x12_consolas, SCREEN_YELLOW, SCREEN_Nor);

    SCREEN_RefreshScreen();
}

/**
 * @brief Page2 - 按钮B页面
 */
void Page2_ButtonB(void)
{
    SCREEN_FillScreen(SCREEN_BLACK);

    // 标题
    SCREEN_DrawString(45, 20, "Page 2", &Font_8x12_consolas, SCREEN_GREEN, SCREEN_Nor);

    // 按钮 "B"
    struUI_Button_t buttonB = {
        .location = {64, 70},    // 中心坐标
        .frame = {40, 30, 5},   // 长度40, 宽度30, 圆角5
        .label = "B",
        .ascii_font = &Font_8x16_consolas,
        .hz_font = NULL,
        .state = 0x00
    };
    SCREEN_DrawButton(&buttonB, SCREEN_RED, SCREEN_Nor);

    // 提示返回
    SCREEN_DrawString(10, 110, "Press A back", &Font_8x12_consolas, SCREEN_YELLOW, SCREEN_Nor);

    SCREEN_RefreshScreen();
}

/**
 * @brief 根据输入切换页面
 * @param input 输入结构体指针
 */
// void Page_Switch(struINPUT_t *input)
void Page_Switch(void)
{
    Screen_GetInput(&struINPUT[0]);

    if (struINPUT[0].value == 0 ) {  // 按键
        return;
    }

    // 检测按键按下 (value=1)
    if (struINPUT[0].value) {
        switch (current_page) {
            case 0:  // 欢迎页
                if (struINPUT[0].value == KEY_Event_ShortPress) {
                    current_page = 1;
                    Page1_ButtonA();
                } else if (struINPUT[0].value == KEY_Event_LongPress) {
                    current_page = 2;
                    Page2_ButtonB();
                }
                break;

            case 1:  // Page1
                if (struINPUT[0].value == KEY_Event_LongPress) {  // 按B返回欢迎页
                    current_page = 0;
                    Page0_Welcome();
                }
                break;

            case 2:  // Page2
                if (struINPUT[0].value == KEY_Event_ShortPress) {  // 按A返回欢迎页
                    current_page = 0;
                    Page0_Welcome();
                }
                break;
        }

        Screen_ClearInput(&struINPUT[0]);
    }
}

/**
 * @brief 初始化页面系统
 */
void Page_Init(void)
{
    current_page = 0;
    Page0_Welcome();
}


void LCD_Test(){
    // ========== 测试1: 颜色填充测试 ==========
    printf("=== Test 1: Color Fill ===\n");

    // 黑色
    SCREEN_FillScreen(SCREEN_BLACK);
    SCREEN_RefreshScreen();
    printf("Black T=%4lu,Interval=%4lu\n", SCREEN_GetRefreshTime(), SCREEN_GetRefreshIntervalTime()); // 这个数据无意义
    R_BSP_SoftwareDelay(500U, BSP_DELAY_UNITS_MILLISECONDS);

    // 红色
    SCREEN_FillScreen(SCREEN_RED);
    SCREEN_RefreshScreen();
    printf("Red T=%4lu,Interval=%4lu\n", SCREEN_GetRefreshTime(), SCREEN_GetRefreshIntervalTime());
    R_BSP_SoftwareDelay(500U, BSP_DELAY_UNITS_MILLISECONDS);

    // 绿色
    SCREEN_FillScreen(SCREEN_GREEN);
    SCREEN_RefreshScreen();
    printf("Green T=%4lu,Interval=%4lu\n", SCREEN_GetRefreshTime(), SCREEN_GetRefreshIntervalTime());
    R_BSP_SoftwareDelay(500U, BSP_DELAY_UNITS_MILLISECONDS);

    // 蓝色
    SCREEN_FillScreen(SCREEN_BLUE);
    SCREEN_RefreshScreen();
    printf("Blue T=%4lu,Interval=%4lu\n", SCREEN_GetRefreshTime(), SCREEN_GetRefreshIntervalTime());
    R_BSP_SoftwareDelay(500U, BSP_DELAY_UNITS_MILLISECONDS);

    // 白色
    SCREEN_FillScreen(SCREEN_WHITE);
    SCREEN_RefreshScreen();
    printf("White T=%4lu,Interval=%4lu\n", SCREEN_GetRefreshTime(), SCREEN_GetRefreshIntervalTime());
    R_BSP_SoftwareDelay(500U, BSP_DELAY_UNITS_MILLISECONDS);

    // ========== 测试2: 线条绘制测试 ==========
    printf("=== Test 2: Lines ===\n");
    SCREEN_FillScreen(SCREEN_BLACK);

    // 绘制各种颜色的线条
    SCREEN_DrawLine(10, 117, 10, 117, SCREEN_RED, SCREEN_Nor);      // 水平红线
    SCREEN_DrawLine(10, 117, 30, 10, SCREEN_GREEN, SCREEN_Nor);     // 垂直绿线
    SCREEN_DrawLine(10, 10, 117, 117, SCREEN_BLUE, SCREEN_Nor);     // 对角蓝线
    SCREEN_DrawLine(117, 10, 10, 117, SCREEN_YELLOW, SCREEN_Nor);    // 反向对角黄线

    SCREEN_RefreshScreen();
    printf("Lines T=%4lu,Interval=%4lu\n", SCREEN_GetRefreshTime(), SCREEN_GetRefreshIntervalTime());
    R_BSP_SoftwareDelay(1000U, BSP_DELAY_UNITS_MILLISECONDS);

    // ========== 测试3: 矩形绘制测试 ==========
    printf("=== Test 3: Rectangles ===\n");
    SCREEN_FillScreen(SCREEN_BLACK);

    // 实心矩形
    SCREEN_DrawRectSolid(5, 40, 5, 40, SCREEN_RED, SCREEN_Nor);       // 左上红色实心
    SCREEN_DrawRectSolid(50, 85, 5, 40, SCREEN_GREEN, SCREEN_Nor);     // 右上绿色实心
    SCREEN_DrawRectSolid(5, 40, 50, 90, SCREEN_BLUE, SCREEN_Nor);      // 左下蓝色实心
    SCREEN_DrawRectSolid(50, 85, 50, 90, SCREEN_YELLOW, SCREEN_Nor);  // 右下黄色实心

    // 空心矩形
    SCREEN_DrawRectHollow(90, 125, 90, 125, SCREEN_WHITE, SCREEN_Nor); // 右下角白色空心框

    SCREEN_RefreshScreen();
    printf("Rectangles T=%4lu,Interval=%4lu\n", SCREEN_GetRefreshTime(), SCREEN_GetRefreshIntervalTime());
    R_BSP_SoftwareDelay(1000U, BSP_DELAY_UNITS_MILLISECONDS);

    // ========== 测试4: 圆弧绘制测试 ==========
    printf("=== Test 4: Arcs ===\n");
    SCREEN_FillScreen(SCREEN_BLACK);

    // 绘制四个象限的圆弧
    SCREEN_DrawQuarArc(64, 64, 50, SCREEN_Quarter1, SCREEN_RED, SCREEN_Nor);    // 第一象限
    SCREEN_DrawQuarArc(64, 64, 50, SCREEN_Quarter2, SCREEN_GREEN, SCREEN_Nor);  // 第二象限
    SCREEN_DrawQuarArc(64, 64, 50, SCREEN_Quarter3, SCREEN_BLUE, SCREEN_Nor);   // 第三象限
    SCREEN_DrawQuarArc(64, 64, 50, SCREEN_Quarter4, SCREEN_YELLOW, SCREEN_Nor); // 第四象限

    SCREEN_RefreshScreen();
    printf("Arcs T=%4lu,Interval=%4lu\n", SCREEN_GetRefreshTime(), SCREEN_GetRefreshIntervalTime());
    R_BSP_SoftwareDelay(1000U, BSP_DELAY_UNITS_MILLISECONDS);

    // ========== 测试5: 文字绘制测试 ==========
    printf("=== Test 5: Text ===\n");
    SCREEN_FillScreen(SCREEN_BLACK);

    // 绘制ASCII字符串
    SCREEN_DrawString(5, 10, "Hello RA4M2!", &Font_8x16_consolas, SCREEN_WHITE, SCREEN_Nor);
    SCREEN_DrawString(5, 30, "ST7735 LCD", &Font_8x16_consolas, SCREEN_GREEN, SCREEN_Nor);
    SCREEN_DrawString(5, 50, "128x128 Pixels", &Font_8x12_consolas, SCREEN_CYAN, SCREEN_Nor);

    // 绘制UTF-8中文字符串
    SCREEN_DrawUTF8String(5, 70, "瑞萨RA4M2", &Font_8x12_consolas, &Font_UTF_16x16_YuMincho, SCREEN_YELLOW, SCREEN_Nor);
    SCREEN_DrawUTF8String(5, 90, "SPI屏幕测试", &Font_8x12_consolas, &Font_UTF_16x16_YuMincho, SCREEN_MAGENTA, SCREEN_Nor);

    SCREEN_RefreshScreen();
    printf("Text T=%4lu,Interval=%4lu\n", SCREEN_GetRefreshTime(), SCREEN_GetRefreshIntervalTime());
    R_BSP_SoftwareDelay(2000U, BSP_DELAY_UNITS_MILLISECONDS);

    // ========== 测试6: 图片绘制测试 ==========
    printf("=== Test 6: Image ===\n");
    SCREEN_FillScreen(SCREEN_BLACK);

    // 绘制苹果图标（单色图片）
    SCREEN_DrawImage(50, 50, 16, 16, gImage_apple, SCREEN_RED, SCREEN_Nor);

    SCREEN_DrawString(45, 90, "Apple", &Font_8x12_consolas, SCREEN_WHITE, SCREEN_Nor);

    SCREEN_RefreshScreen();
    printf("Image T=%4lu,Interval=%4lu\n", SCREEN_GetRefreshTime(), SCREEN_GetRefreshIntervalTime());
    R_BSP_SoftwareDelay(1000U, BSP_DELAY_UNITS_MILLISECONDS);

    // ========== 测试7: 帧率测试 ==========
    printf("=== Test 7: FPS Test ===\n");
    SCREEN_FillScreen(SCREEN_BLACK);
    SCREEN_DrawString(5, 5, "FPS Test", &Font_8x16_consolas, SCREEN_WHITE, SCREEN_Nor);

    for(int i = 0; i < 10; i++) {
        SCREEN_DrawRectSolid(20 + i*10, 30 + i*10, 20, 100, SCREEN_COLOR565(i*25, 255-i*25, 128), SCREEN_Nor);
        SCREEN_RefreshScreen();
        printf("Frame %d: T=%4lu,Interval=%4lu\n", i+1, SCREEN_GetRefreshTime(), SCREEN_GetRefreshIntervalTime());
    }

    R_BSP_SoftwareDelay(1000U, BSP_DELAY_UNITS_MILLISECONDS);

    // ========== 测试完成 ==========
    printf("=== LCD Test Complete ===\n");
    SCREEN_FillScreen(SCREEN_BLACK);
    SCREEN_DrawUTF8String(30, 55, "测试完成", &Font_8x12_consolas, &Font_UTF_16x16_YuMincho, SCREEN_GREEN, SCREEN_Nor);
    SCREEN_RefreshScreen();
}

