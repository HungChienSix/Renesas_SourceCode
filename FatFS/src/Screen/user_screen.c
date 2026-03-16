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
    .text = "Music Player",
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
 * @brief 主界面(显示歌曲的列表)
 */
void Page1_Main(void)
{
    uint8_t page_id = 0;

    SCREEN_FillScreen(SCREEN_BLACK);

    // 获取播放列表头部和总数
    struAudio_t *playlist_head = I2S_GetPlaylistHead();
    uint16_t total_songs = I2S_GetPlaylistCount();

    if (playlist_head == NULL || total_songs == 0)
    {
        // 没有歌曲时显示提示
        SCREEN_DrawString(32, 60, "No Songs", &Font_8x12_consolas, SCREEN_WHITE, SCREEN_Nor);
    }
    else
    {
        // 确保有选中的歌曲不为NULL
        if (g_sys_info.selected_audio == NULL) {
            g_sys_info.selected_audio = I2S_GetPlaylistHead();
        }

        // 如果有选中的歌曲，计算其所在页码
        if (g_sys_info.selected_audio != NULL)
        {
            page_id = g_sys_info.selected_audio->id / SONGS_PER_PAGE;
        }

        uint8_t total_pages = (total_songs + SONGS_PER_PAGE - 1) / SONGS_PER_PAGE;

        // 计算当前页的歌曲索引范围
        uint16_t page_start_index = page_id * SONGS_PER_PAGE;
        uint16_t page_end_index = page_start_index + SONGS_PER_PAGE;
        if (page_end_index > total_songs) {
            page_end_index = total_songs;
        }

        // 显示标题和分页信息
        char page_text[20];
        snprintf(page_text, sizeof(page_text), "P%d/%d", page_id + 1, total_pages);
        SCREEN_DrawString(95, 2, page_text, &Font_8x12_consolas, SCREEN_WHITE, SCREEN_Nor);

        // 显示当前页的歌曲
        uint8_t y_pos = 20;
        struAudio_t *current_song = playlist_head;

        // 跳转到当前页的起始歌曲
        for (uint16_t i = 0; i < page_start_index && current_song != NULL; i++) {
            current_song = current_song->next;
        }

        // 显示当前页的歌曲（最多4首）
        for (uint16_t i = page_start_index; i < page_end_index && current_song != NULL; i++)
        {
            // 如果是选中的歌曲，用不同颜色显示
            SCREEN_Pixel_t song_color = (i == g_sys_info.selected_audio->id) ? SCREEN_YELLOW : SCREEN_WHITE;

            // 第一行：绘制序号
            char num_text[5];
            snprintf(num_text, sizeof(num_text), "%u.", i + 1);
            SCREEN_DrawString(4, y_pos, num_text, &Font_8x12_consolas, song_color, SCREEN_Nor);

            // 计算第一行歌曲名可用空间（从x=20开始，第二行从x=122开始留6个字符长度）
            // 8x12字体：每个字符8像素宽，6个字符=48像素，所以第二行从x=122开始
            uint8_t first_line_max_chars = (122 - 20) / 8;  // 102像素 / 8 = 12个字符

            // 绘制歌曲名：分为两段（第一段 + 第二段前半部分）
            uint8_t total_name_len = 0;
            for (uint8_t j = 0; current_song->name[j] != '\0'; j++) {
                total_name_len++;
            }

            // 第一段：x=20-116，最多12个字符
            uint8_t first_line_len = (total_name_len > 12) ? 12 : total_name_len;
            char first_line[13];
            uint8_t i;
            for (i = 0; i < first_line_len && i < 12; i++) {
                first_line[i] = current_song->name[i];
            }
            first_line[i] = '\0';
            if (first_line_len > 0) {
                SCREEN_DrawString(20, y_pos, first_line, &Font_8x12_consolas, song_color, SCREEN_Nor);
            }

            // 第二段前半部分：x=0-80，最多10个字符
            if (total_name_len > 12) {
                char second_part[11];
                uint8_t second_part_len = (total_name_len - 12 > 10) ? 10 : (total_name_len - 12);
                for (i = 0; i < second_part_len; i++) {
                    second_part[i] = current_song->name[12 + i];
                }
                second_part[i] = '\0';
                SCREEN_DrawString(0, y_pos + 12, second_part, &Font_8x12_consolas, song_color, SCREEN_Nor);
            }

            // 第二行：显示歌曲时长信息（固定在 x=88）
            y_pos += 12;
            char duration_text[6];
            uint32_t total_seconds = current_song->total_samples / current_song->fmt.samplesPerSec;
            uint8_t minutes = total_seconds / 60;
            uint8_t seconds = total_seconds % 60;
            snprintf(duration_text, sizeof(duration_text), "%u:%02u", minutes, seconds);
            SCREEN_DrawString(88, y_pos, duration_text, &Font_8x12_consolas, SCREEN_WHITE, SCREEN_Nor);

            y_pos += 10;  // 下一首歌曲的起始位置

            current_song = current_song->next;
        }

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
        SCREEN_DrawString(5, 2, "UI Test", &Font_8x12_consolas, SCREEN_WHITE, SCREEN_Nor);

        // 测试1: 按钮（未按下）
        struUI_Button_t btn_idle = {
            .location = {40, 30},
            .frame = {60, 20, 4},
            .label = "Idle",
            .ascii_font = &Font_8x12_consolas,
            .hz_font = &Font_UTF_16x16_YuMincho,
            .color = {SCREEN_WHITE, SCREEN_BLUE, SCREEN_WHITE},  // 边框, 填充, 文本
            .state = 0x00
        };
        SCREEN_DrawButton(&btn_idle);

        // 测试2: 按钮（按下）
        struUI_Button_t btn_pressed = {
            .location = {40, 58},
            .frame = {60, 20, 4},
            .label = "Pressed",
            .ascii_font = &Font_8x12_consolas,
            .hz_font = &Font_UTF_16x16_YuMincho,
            .color = {SCREEN_WHITE, SCREEN_BLUE, SCREEN_WHITE},  // 边框, 填充, 文本
            .state = 0xFF
        };
        SCREEN_DrawButton(&btn_pressed);

        // 测试3: 提示框
        struUI_Tooltip_t tooltip = {
            .location = {40, 90},
            .frame = {70, 24},
            .text = "Info Box",
            .ascii_font = &Font_8x12_consolas,
            .hz_font = &Font_UTF_16x16_YuMincho,
            .color = {SCREEN_BLUE, SCREEN_WHITE}  // 背景颜色, 文本颜色
        };
        SCREEN_DrawTooltip(&tooltip);

        SCREEN_RefreshScreen();
        printf("UI Components T=%4lu,Interval=%4lu\n", SCREEN_GetRefreshTime(), SCREEN_GetRefreshIntervalTime());
        R_BSP_SoftwareDelay(2000U, BSP_DELAY_UNITS_MILLISECONDS);
    }
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
        SCREEN_DrawString(4, 56, "Name:", &Font_8x12_consolas, SCREEN_GREEN, SCREEN_Nor);
        SCREEN_DrawString(32, 56, audio->name, &Font_8x12_consolas, SCREEN_WHITE, SCREEN_Nor);

        // 显示文件名
        SCREEN_DrawString(4, 70, "File:", &Font_8x12_consolas, SCREEN_GREEN, SCREEN_Nor);
        SCREEN_DrawString(32, 70, audio->filename, &Font_8x12_consolas, SCREEN_WHITE, SCREEN_Nor);

        // 计算时长
        uint32_t total_seconds = audio->total_samples / audio->fmt.samplesPerSec;
        uint8_t minutes = total_seconds / 60;
        uint8_t seconds = total_seconds % 60;
        char duration_text[10];
        snprintf(duration_text, sizeof(duration_text), "%u:%02u", minutes, seconds);

        // 显示时长
        SCREEN_DrawString(4, 84, "Time:", &Font_8x12_consolas, SCREEN_GREEN, SCREEN_Nor);
        SCREEN_DrawString(32, 84, duration_text, &Font_8x12_consolas, SCREEN_WHITE, SCREEN_Nor);

        // 显示采样率
        char sample_rate_text[12];
        snprintf(sample_rate_text, sizeof(sample_rate_text), "%luHz", audio->fmt.samplesPerSec);
        SCREEN_DrawString(4, 98, "Rate:", &Font_8x12_consolas, SCREEN_GREEN, SCREEN_Nor);
        SCREEN_DrawString(32, 98, sample_rate_text, &Font_8x12_consolas, SCREEN_WHITE, SCREEN_Nor);

        // 显示位深
        char bits_text[8];
        snprintf(bits_text, sizeof(bits_text), "%ubit", audio->fmt.bits_per_sample);
        SCREEN_DrawString(80, 84, "Bit:", &Font_8x12_consolas, SCREEN_GREEN, SCREEN_Nor);
        SCREEN_DrawString(104, 84, bits_text, &Font_8x12_consolas, SCREEN_WHITE, SCREEN_Nor);

        // 显示声道数
        char channels_text[8];
        snprintf(channels_text, sizeof(channels_text), "%uch", audio->fmt.channels);
        SCREEN_DrawString(80, 98, "CH:", &Font_8x12_consolas, SCREEN_GREEN, SCREEN_Nor);
        SCREEN_DrawString(100, 98, channels_text, &Font_8x12_consolas, SCREEN_WHITE, SCREEN_Nor);

        // 显示文件大小
        char size_text[12];
        uint32_t size_kb = audio->file_size / 1024;
        snprintf(size_text, sizeof(size_text), "%luKB", size_kb);
        SCREEN_DrawString(4, 112, "Size:", &Font_8x12_consolas, SCREEN_GREEN, SCREEN_Nor);
        SCREEN_DrawString(32, 112, size_text, &Font_8x12_consolas, SCREEN_WHITE, SCREEN_Nor);
    }
    else
    {
        // 没有选中歌曲时显示提示
        SCREEN_DrawString(32, 60, "No Song", &Font_8x12_consolas, SCREEN_WHITE, SCREEN_Nor);
        SCREEN_DrawString(32, 72, "Selected", &Font_8x12_consolas, SCREEN_WHITE, SCREEN_Nor);
    }

    SCREEN_RefreshScreen();
}
