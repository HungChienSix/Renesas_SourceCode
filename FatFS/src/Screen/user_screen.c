#include "user_screen.h"
#include <stdio.h>

#define SONGS_PER_PAGE 4
extern sys_info g_sys_info ;

/**
 * @brief 开屏界面
 */
static struUI_Tooltip_t tooltip_splash = {
    .location = {64, 64},
    .frame = {110, 36},
    .text = "Press any key",
    .ascii_font = &Font_8x16_consolas,
    .hz_font = &Font_UTF_16x12_YuMincho,
    .color = {SCREEN_BLUE, SCREEN_GREEN}  // 背景颜色, 文本颜色
};

void Page0_Welcome(void)
{
    SCREEN_FillScreen(SCREEN_BLACK);
    SCREEN_DrawTooltip(&tooltip_splash);
    SCREEN_RefreshScreen();
}

/**
 * @brief 绘制多行UTF文本（自动换行）
 * @param x 起始x坐标
 * @param y 起始y坐标
 * @param text 要显示的文本
 * @param ascii_font ASCII字体
 * @param hz_font 中文UTF字体
 * @param color 颜色
 * @param max_width 每行最大宽度（像素）
 * @param line_height 行高
 */
static void DrawMultiLineText(int16_t x, int16_t y, const char *text,
                               const struFont_t *ascii_font,
                               const struFont_UTF_t *hz_font,
                               SCREEN_Pixel_t color,
                               uint8_t max_width, uint8_t line_height)
{
    char line_buffer[64];
    uint8_t line_index = 0;
    int16_t current_y = y;
    uint16_t line_width = 0;  // 当前行已用宽度
    uint8_t i = 0;

    while (text[i] != '\0')
    {
        // 计算当前字符的宽度
        uint8_t char_width = 8;  // ASCII字符默认8像素

        // UTF-8中文判断：0xE0以上为3字节中文（GBK/GB2312兼容）
        if ((text[i] & 0x80) != 0)
        {
            char_width = 16;  // UTF字符16像素

            // 复制UTF字符（3字节）
            if (line_index + 3 < sizeof(line_buffer))
            {
                line_buffer[line_index++] = text[i++];
                line_buffer[line_index++] = text[i++];
                line_buffer[line_index++] = text[i++];
            }
        }
        else
        {
            // ASCII字符（1字节）
            if (line_index + 1 < sizeof(line_buffer))
            {
                line_buffer[line_index++] = text[i++];
            }
        }

        line_width += char_width;

        // 超过最大宽度或遇到换行符，则输出一行
        if (line_width > max_width || (text[i] == '\n' && text[i] != '\0'))
        {
            // 如果是换行符，跳过它
            if (text[i] == '\n')
            {
                i++;
            }

            line_buffer[line_index] = '\0';
            SCREEN_DrawUTFString(x, current_y, line_buffer, ascii_font, hz_font, color, SCREEN_Nor);
            current_y += line_height;
            line_index = 0;
            line_width = 0;
        }
    }

    // 输出最后一行
    if (line_index > 0)
    {
        line_buffer[line_index] = '\0';
        SCREEN_DrawUTFString(x, current_y, line_buffer, ascii_font, hz_font, color, SCREEN_Nor);
    }
}

/**
 * @brief 主界面(显示上一首、当前、下一首歌曲)
 */
void Page1_Main(void)
{
    SCREEN_FillScreen(SCREEN_BLACK);

    if (g_sys_info.selected_audio != NULL)
    {
        struAudio_t *prev = g_sys_info.selected_audio->prev;
        struAudio_t *curr = g_sys_info.selected_audio;
        struAudio_t *next = g_sys_info.selected_audio->next;

        // 顶部进度条区域
        // 计算当前播放进度
        uint32_t current_seconds = curr->current_sample / curr->fmt.samplesPerSec;
        uint32_t total_seconds = curr->total_samples / curr->fmt.samplesPerSec;
        uint8_t current_min = current_seconds / 60;
        uint8_t current_sec = current_seconds % 60;
        uint8_t total_min = total_seconds / 60;
        uint8_t total_sec = total_seconds % 60;

        // 绘制进度条
        uint8_t progress_percent = (uint8_t)((curr->current_sample / curr->total_samples) * 100);
        struUI_ProgressBar_t progress_bar = {
            .location = {64, 8},      // 中心坐标 (128/2=64, 8)
            .frame = {120, 6},         // 宽度120, 高度6
            .color = {SCREEN_WHITE, SCREEN_YELLOW},  // 边框白色, 填充黄色
            .progress = progress_percent
        };
        SCREEN_DrawProgressBar(&progress_bar);

        // 显示当前时间 / 总时间
        char time_text[12];
        snprintf(time_text, sizeof(time_text), "%u:%02u/%u:%02u", current_min, current_sec, total_min, total_sec);
        SCREEN_DrawString(8, 18, time_text, &Font_8x12_consolas, SCREEN_WHITE, SCREEN_Nor);

        // 绘制大圆角矩形框住三首歌
        SCREEN_DrawRoundRectHollow(0, 127, 28, 100, 4, SCREEN_YELLOW, SCREEN_Nor);

        // 上部分：上一首歌曲
        if (prev != NULL)
        {
            DrawMultiLineText(4, 36, prev->name, &Font_8x12_consolas, &Font_UTF_16x12_YuMincho, SCREEN_WHITE, 120, 12);
        }

        // 中部分：当前歌曲 (y=52) - 黄色高亮
        DrawMultiLineText(4, 52, curr->name, &Font_8x16_consolas, &Font_UTF_16x16_YuMincho, SCREEN_YELLOW, 120, 16);

        // 下部分：下一首歌曲 (y=68)
        if (next != NULL)
        {
            DrawMultiLineText(4, 68, next->name, &Font_8x12_consolas, &Font_UTF_16x12_YuMincho, SCREEN_WHITE, 120, 12);
        }
    }
    else
    {
        // 没有选中歌曲
        SCREEN_DrawString(40, 32, "No Songs", &Font_8x12_consolas, SCREEN_WHITE, SCREEN_Nor);
        SCREEN_DrawString(32, 46, "in Playlist", &Font_8x12_consolas, SCREEN_WHITE, SCREEN_Nor);
    }

    SCREEN_RefreshScreen();
}

/**
 * @brief 歌曲详情界面
 */
void Page2_SongInfo(void)
{
    SCREEN_FillScreen(SCREEN_BLACK);

    if (g_sys_info.selected_audio != NULL)
    {
        struAudio_t *audio = g_sys_info.selected_audio;

        // 显示标题
        SCREEN_DrawString(48, 2, "Song Info", &Font_8x12_consolas, SCREEN_WHITE, SCREEN_Nor);

        // 占位专辑封面（居中显示）
        SCREEN_DrawRGBImage(128-32, 16, 32, 32, gImage_RGB_163music, SCREEN_Nor);

        // 显示歌曲名
        // SCREEN_DrawString(4, 56, "Name:", &Font_8x12_consolas, SCREEN_GREEN, SCREEN_Nor);
        SCREEN_DrawString(0, 56, audio->name, &Font_8x12_consolas, SCREEN_WHITE, SCREEN_Nor);

        // 显示文件名
        // SCREEN_DrawString(4, 70, "File:", &Font_8x12_consolas, SCREEN_GREEN, SCREEN_Nor);
        SCREEN_DrawString(0, 70, audio->filename, &Font_8x12_consolas, SCREEN_WHITE, SCREEN_Nor);

        // 计算时长
        uint32_t total_seconds = audio->total_samples / audio->fmt.samplesPerSec;
        uint8_t minutes = total_seconds / 60;
        uint8_t seconds = total_seconds % 60;
        char duration_text[10];
        snprintf(duration_text, sizeof(duration_text), "%u:%02u", minutes, seconds);

        // 显示时长
        // SCREEN_DrawString(4, 84, "Time:", &Font_8x12_consolas, SCREEN_GREEN, SCREEN_Nor);
        SCREEN_DrawString(0, 84, duration_text, &Font_8x12_consolas, SCREEN_WHITE, SCREEN_Nor);

        // 显示采样率
        char sample_rate_text[12];
        snprintf(sample_rate_text, sizeof(sample_rate_text), "%luHz", audio->fmt.samplesPerSec);
        // SCREEN_DrawString(4, 98, "Rate:", &Font_8x12_consolas, SCREEN_GREEN, SCREEN_Nor);
        SCREEN_DrawString(0, 98, sample_rate_text, &Font_8x12_consolas, SCREEN_WHITE, SCREEN_Nor);

        // 显示位深
        char bits_text[8];
        snprintf(bits_text, sizeof(bits_text), "%ubit", audio->fmt.bits_per_sample);
        // SCREEN_DrawString(80, 84, "Bit:", &Font_8x12_consolas, SCREEN_GREEN, SCREEN_Nor);
        SCREEN_DrawString(80, 84, bits_text, &Font_8x12_consolas, SCREEN_WHITE, SCREEN_Nor);

        // 显示声道数
        char channels_text[8];
        snprintf(channels_text, sizeof(channels_text), "%uch", audio->fmt.channels);
        // SCREEN_DrawString(80, 98, "CH:", &Font_8x12_consolas, SCREEN_GREEN, SCREEN_Nor);
        SCREEN_DrawString(80, 98, channels_text, &Font_8x12_consolas, SCREEN_WHITE, SCREEN_Nor);

        // 显示文件大小
        char size_text[12];
        uint32_t size_kb = audio->file_size / 1024;
        snprintf(size_text, sizeof(size_text), "%luKB", size_kb);
        // SCREEN_DrawString(4, 112, "Size:", &Font_8x12_consolas, SCREEN_GREEN, SCREEN_Nor);
        SCREEN_DrawString(0, 112, size_text, &Font_8x12_consolas, SCREEN_WHITE, SCREEN_Nor);
    }
    else
    {
        // 没有选中歌曲时显示提示
        SCREEN_DrawString(32, 60, "No Song", &Font_8x12_consolas, SCREEN_WHITE, SCREEN_Nor);
        SCREEN_DrawString(32, 72, "Selected", &Font_8x12_consolas, SCREEN_WHITE, SCREEN_Nor);
    }

    SCREEN_RefreshScreen();
}

/**
 * @brief LCD测试函数，根据test_id执行不同的测试
 * @param test_id 测试ID: 1=线/矩形/弧测试, 2=UI组件测试
 */
void LCD_Test(uint8_t test_id)
{
    if (test_id == 1)
    {
        // ========== 测试1: 线条/矩形/圆弧绘制测试 ==========
        printf("=== Test 1: Shapes ===\n");
        SCREEN_FillScreen(SCREEN_BLACK);

        // 绘制各种颜色的线条
        SCREEN_DrawLine(10, 117, 10, 10, SCREEN_RED, SCREEN_Nor);        // 水平红线
        SCREEN_DrawLine(10, 10, 10, 117, SCREEN_GREEN, SCREEN_Nor);      // 垂直绿线
        SCREEN_DrawLine(10, 117, 10, 117, SCREEN_BLUE, SCREEN_Nor);      // 对角蓝线
        SCREEN_DrawLine(117, 10, 10, 117, SCREEN_YELLOW, SCREEN_Nor);   // 反向对角黄线

        // 绘制实心矩形
        SCREEN_DrawRectSolid(5, 40, 5, 40, SCREEN_RED, SCREEN_Nor);       // 左上红色实心
        SCREEN_DrawRectSolid(50, 85, 5, 40, SCREEN_GREEN, SCREEN_Nor);     // 右上绿色实心
        SCREEN_DrawRectSolid(5, 40, 50, 90, SCREEN_BLUE, SCREEN_Nor);      // 左下蓝色实心
        SCREEN_DrawRectSolid(50, 85, 50, 90, SCREEN_YELLOW, SCREEN_Nor);  // 右下黄色实心

        // 绘制空心矩形
        SCREEN_DrawRectHollow(90, 125, 90, 125, SCREEN_WHITE, SCREEN_Nor); // 右下角白色空心框

        // 绘制四个象限的圆弧
        SCREEN_DrawQuarArc(64, 64, 50, SCREEN_Quarter1, SCREEN_RED, SCREEN_Nor);    // 第一象限
        SCREEN_DrawQuarArc(64, 64, 50, SCREEN_Quarter2, SCREEN_GREEN, SCREEN_Nor);  // 第二象限
        SCREEN_DrawQuarArc(64, 64, 50, SCREEN_Quarter3, SCREEN_BLUE, SCREEN_Nor);   // 第三象限
        SCREEN_DrawQuarArc(64, 64, 50, SCREEN_Quarter4, SCREEN_YELLOW, SCREEN_Nor); // 第四象限

        SCREEN_RefreshScreen();
        printf("Shapes T=%4lu,Interval=%4lu\n", SCREEN_GetRefreshTime(), SCREEN_GetRefreshIntervalTime());
        R_BSP_SoftwareDelay(2000U, BSP_DELAY_UNITS_MILLISECONDS);
    }
    else if (test_id == 2)
    {
        // ========== 测试2: UI组件测试 ==========
        printf("=== Test 2: UI Components ===\n");
        SCREEN_FillScreen(SCREEN_BLACK);

        // 1. 测试 Tooltip (文本提示框)
        struUI_Tooltip_t tooltip1 = {
            .location = {64, 20},
            .frame = {120, 28},
            .text = "UI Components",
            .ascii_font = &Font_8x12_serif,
            .hz_font = &Font_UTF_16x12_YuMincho,
            .color = {SCREEN_BLUE, SCREEN_WHITE}  // 背景颜色, 文本颜色
        };
        SCREEN_DrawTooltip(&tooltip1);

        // 2. 测试 Button - 未按下状态
        struUI_Button_t button1 = {
            .location = {32, 60},
            .frame = {50, 24, 4},  // 长度, 宽度, 圆角半径
            .label = "Play",
            .ascii_font = &Font_8x12_serif,
            .hz_font = &Font_UTF_16x12_YuMincho,
            .color = {SCREEN_GREEN, SCREEN_BLUE, SCREEN_WHITE},  // 边框, 填充, 文本
            .state = 0x00  // 未按下
        };
        SCREEN_DrawButton(&button1);

        // 3. 测试 Button - 按下状态
        struUI_Button_t button2 = {
            .location = {96, 60},
            .frame = {50, 24, 4},
            .label = "Stop",
            .ascii_font = &Font_8x12_serif,
            .hz_font = &Font_UTF_16x12_YuMincho,
            .color = {SCREEN_RED, SCREEN_BLUE, SCREEN_WHITE},
            .state = 0xFF  // 按下
        };
        SCREEN_DrawButton(&button2);

        // 4. 测试 Button - 中文标签
        struUI_Button_t button3 = {
            .location = {64, 95},
            .frame = {70, 24, 4},
            .label = "AB清明",
            .ascii_font = &Font_8x12_serif,
            .hz_font = &Font_UTF_16x12_YuMincho,
            .color = {SCREEN_YELLOW, SCREEN_RED, SCREEN_GREEN},
            .state = 0x00
        };
        SCREEN_DrawButton(&button3);

        // 5. 测试 ProgressBar - 不同进度
        struUI_ProgressBar_t progress1 = {
            .location = {64, 120},
            .frame = {100, 10},
            .color = {SCREEN_WHITE, SCREEN_CYAN},  // 边框颜色, 填充颜色
            .progress = 65
        };
        SCREEN_DrawProgressBar(&progress1);

        // 6. 测试 Tooltip - 带阴影
        struUI_Tooltip_t tooltip2 = {
            .location = {64, 150},
            .frame = {80, 20},
            .text = "Progress: 65%",
            .ascii_font = &Font_8x12_serif,
            .hz_font = &Font_UTF_16x12_YuMincho,
            .color = {SCREEN_RED, SCREEN_YELLOW}
        };
        SCREEN_DrawTooltip(&tooltip2);

        SCREEN_RefreshScreen();
        printf("UI Components Test T=%4lu,Interval=%4lu\n", SCREEN_GetRefreshTime(), SCREEN_GetRefreshIntervalTime());
        R_BSP_SoftwareDelay(2000U, BSP_DELAY_UNITS_MILLISECONDS);
    }
    else if (test_id == 3){
        // ========== 测试3: 字体测试 ==========
        printf("=== Font Test ===\n");

        // 清屏并显示标题
        SCREEN_FillScreen(SCREEN_BLACK);

        // 测试1: 8x16 ASCII Consolas
        SCREEN_DrawString(2, 0, "ABCDEFGHIJKLM", &Font_8x16_consolas, SCREEN_WHITE, SCREEN_Nor);

        // 测试2: 8x12 ASCII Consolas
        SCREEN_DrawString(2, 20, "1234567890", &Font_8x12_consolas, SCREEN_WHITE, SCREEN_Nor);

        // 测试3: 8x16 Serif
        SCREEN_DrawString(2, 40, "ABC123", &Font_8x16_serif, SCREEN_WHITE, SCREEN_Nor);

        // 测试4: 8x12 Serif
        SCREEN_DrawString(2, 60, "XYZ789", &Font_8x12_serif, SCREEN_WHITE, SCREEN_Nor);

        // 测试5: 16x16 UTF 中文（游明朝）
        SCREEN_DrawUTFString(2, 80, "你好", &Font_8x16_consolas, &Font_UTF_16x16_YuMincho, SCREEN_WHITE, SCREEN_Nor);

        // 测试6: 16x12 UTF 中文（游明朝）
        SCREEN_DrawUTFString(2, 100, "你好", &Font_8x16_consolas, &Font_UTF_16x12_YuMincho, SCREEN_WHITE, SCREEN_Nor);

        // 绘制分割线
        SCREEN_DrawLine(0, 128, 0, 128, SCREEN_WHITE, SCREEN_Nor);

        SCREEN_RefreshScreen();
        printf("Font Test T=%4lu,Interval=%4lu\n", SCREEN_GetRefreshTime(), SCREEN_GetRefreshIntervalTime());
        R_BSP_SoftwareDelay(1000U, BSP_DELAY_UNITS_MILLISECONDS);  // 显示1秒
    }
}
