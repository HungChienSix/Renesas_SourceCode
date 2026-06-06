/* V2.0 Screen - Upper Layer Abstract */

#include "screen.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdlib.h>
#include <string.h>

/* F-12 (并发保护) — 当前未启用:
 *   本项目 configUSE_MUTEXES=0 / configUSE_RECURSIVE_MUTEXES=0
 *   (FSP 重新生成 FreeRTOSConfig.h 后保持关闭)
 *   后果: 无法用 xSemaphoreCreate(Mutex|RecursiveMutex)Static 互斥绘制/刷新
 *   现状约束: 假设单一生产者 (仅 spi_screen_thread 调 SCREEN_*)
 *   未来如需多任务并发画图, 把 configUSE_RECURSIVE_MUTEXES 改 1, 取消下面注释:
 *
 *   #include "FreeRTOS.h"
 *   #include "semphr.h"
 *   static StaticSemaphore_t g_screen_mutex_buf;
 *   static SemaphoreHandle_t g_screen_mutex = NULL;
 *   ...
 *   g_screen_mutex = xSemaphoreCreateRecursiveMutexStatic(&g_screen_mutex_buf);
 *   xSemaphoreTakeRecursive(g_screen_mutex, portMAX_DELAY);
 *   xSemaphoreGiveRecursive(g_screen_mutex);
 */

/* 帧缓冲区: 动态尺寸, 16位色 (RGB565) */
static uint16_t display_ram[SCREEN_Height][SCREEN_Width] = {0x0000};

#ifdef SCREEN_PARTIAL_REFRESH
/* 行级脏标记 */
static bool dirty_row[SCREEN_Height] = {false};
static uint16_t dirty_col_min[SCREEN_Height];
static uint16_t dirty_col_max[SCREEN_Height];
static uint8_t batch_buf[SCREEN_Width * 2];

static inline void mark_dirty(int16_t y, int16_t x) {
    if (!dirty_row[y]) {
        dirty_col_min[y] = (uint16_t)x;
        dirty_col_max[y] = (uint16_t)x;
        dirty_row[y] = true;
    } else {
        if ((uint16_t)x < dirty_col_min[y]) dirty_col_min[y] = (uint16_t)x;
        if ((uint16_t)x > dirty_col_max[y]) dirty_col_max[y] = (uint16_t)x;
    }
}

static inline void mark_dirty_range(int16_t y, int16_t x0, int16_t x1) {
    if (!dirty_row[y]) {
        dirty_col_min[y] = (uint16_t)x0;
        dirty_col_max[y] = (uint16_t)x1;
        dirty_row[y] = true;
    } else {
        if ((uint16_t)x0 < dirty_col_min[y]) dirty_col_min[y] = (uint16_t)x0;
        if ((uint16_t)x1 > dirty_col_max[y]) dirty_col_max[y] = (uint16_t)x1;
    }
}
#endif

/* ========== 屏幕初始化 ========== */

/* 对外API: 一次性完成屏幕初始化 */
fsp_err_t SCREEN_Init(void) {
    fsp_err_t err = SCREEN_HW_Init();   // 硬件初始化 (宏映射到驱动)
    if (err != FSP_SUCCESS) return err;
    err = SCREEN_ChipInit();            // 驱动芯片初始化 (宏映射到驱动)
    if (err != FSP_SUCCESS) return err;
    /* F-12: 互斥量创建见上方注释块 (当前 configUSE_RECURSIVE_MUTEXES=0, 跳过) */
    SCREEN_Fill(SCREEN_BLACK);
    SCREEN_Refresh_Force();
    return FSP_SUCCESS;
}

/* ========== 帧缓冲读写 ========== */

SCREEN_Pixel_t SCREEN_GetPixel(int16_t x, int16_t y) {
    return display_ram[y][x];
}

void SCREEN_SetPixel(int16_t x, int16_t y, SCREEN_Pixel_t pixel) {
    if ((x < 0) || (y < 0) || (x > SCREEN_Width - 1) || (y > SCREEN_Height - 1))
        return;
    if (display_ram[y][x] == pixel)
        return;
    display_ram[y][x] = pixel;
#ifdef SCREEN_PARTIAL_REFRESH
    mark_dirty(y, x);
#endif
}

/* ========== 刷新函数 ========== */

void SCREEN_Refresh_Force(void) {
    /* F-12: 互斥量禁用 (见文件头注释), 当前假设单生产者调用 SCREEN_Refresh */

    SCREEN_SetWindow(0, 0, SCREEN_Width - 1, SCREEN_Height - 1);
    SCREEN_WriteCmd(SCREEN_CMD_RAMWR);

    static uint8_t buff[SCREEN_Width * 2];
    /* F-10: 连续 N 行 SPI 失败就放弃, 不要让线程在坏总线上自旋至 watchdog */
    const uint32_t per_row_timeout_us = 5000;          // 单行 5ms (正常 1~2ms)
    const uint8_t  max_consecutive_fail = 3;          // 连续 3 行失败即中止
    uint8_t consecutive_fail = 0;

    for (uint16_t h = 0; h < SCREEN_Height; h++) {
        for (uint16_t w = 0; w < SCREEN_Width; w++) {
            buff[w * 2] = (uint8_t)(display_ram[h][w] >> 8);
            buff[w * 2 + 1] = (uint8_t)(display_ram[h][w] & 0xFF);
        }
        int ret = SCREEN_WriteData(buff, (size_t)SCREEN_Width * 2U, per_row_timeout_us);
        if (ret != 0) {
            if (++consecutive_fail >= max_consecutive_fail) {
                goto done;  // 总线可能坏, 不再消耗 CPU
            }
        } else {
            consecutive_fail = 0;
        }
    }
done:
    ;
}

uint32_t SCREEN_Refresh(void) {
    /* F-12: 互斥量禁用 (见文件头注释), 当前假设单生产者调用 SCREEN_Refresh */
    /* 时间测量: 改用 FreeRTOS tick, 1ms 粒度 (HZ=1000), 屏刷通常 1~2ms 够用 */

    TickType_t start_tick = xTaskGetTickCount();

#ifdef SCREEN_PARTIAL_REFRESH
    for (int16_t row = 0; row < SCREEN_Height; row++) {
        if (!dirty_row[row]) continue;

        uint16_t c0 = dirty_col_min[row];
        uint16_t c1 = dirty_col_max[row];

        SCREEN_SetWindow(c0, (uint16_t)row, c1, (uint16_t)row);  /* row 非负 (0..H-1) */
        SCREEN_WriteCmd(SCREEN_CMD_RAMWR);

        size_t idx = 0;
        for (uint16_t w = c0; w <= c1; w++) {
            batch_buf[idx++] = (uint8_t)(display_ram[row][w] >> 8);
            batch_buf[idx++] = (uint8_t)(display_ram[row][w] & 0xFF);
        }
        if (SCREEN_WriteData(batch_buf, idx, 100000U) != 0) {
            return pdTICKS_TO_MS(xTaskGetTickCount() - start_tick);  // F-10: SPI 失败立即返回
        }

        dirty_row[row] = false;
    }
#else
    SCREEN_Refresh_Force();
    return pdTICKS_TO_MS(xTaskGetTickCount() - start_tick);
#endif

    return pdTICKS_TO_MS(xTaskGetTickCount() - start_tick); // 返回的是ms单位
}

/* ========== 基础绘图 ========== */

void SCREEN_Fill(SCREEN_Pixel_t pixel) {
#ifdef SCREEN_PARTIAL_REFRESH
    for (int16_t y = 0; y < SCREEN_Height; y++) {
        int16_t c0 = -1;
        for (int16_t x = 0; x < SCREEN_Width; x++) {
            if (display_ram[y][x] != pixel) { c0 = x; break; }
        }
        if (c0 < 0) continue;

        int16_t c1 = c0;
        for (int16_t x = SCREEN_Width - 1; x > c0; x--) {
            if (display_ram[y][x] != pixel) { c1 = x; break; }
        }

        for (int16_t x = c0; x <= c1; x++)
            display_ram[y][x] = pixel;
        mark_dirty_range(y, c0, c1);
    }
#else
    uint8_t hi = (uint8_t)(pixel >> 8);
    uint8_t lo = (uint8_t)(pixel & 0xFF);
    if (hi == lo) {
        memset(display_ram, hi, sizeof(display_ram));
    } else {
        for (uint32_t i = 0; i < SCREEN_Height * SCREEN_Width; i++) {
            ((SCREEN_Pixel_t *)display_ram)[i] = pixel;
        }
    }
#endif
}

void SCREEN_DrawHLine(int16_t x0, int16_t x1, int16_t y, SCREEN_Pixel_t pixel, SCREEN_Mode_t mode) {
    if (y < 0 || y >= SCREEN_Height) return;
    int16_t xs = (x0 < x1) ? x0 : x1;
    int16_t xe = (x0 < x1) ? x1 : x0;
    if (xs < 0) xs = 0;
    if (xe >= SCREEN_Width) xe = SCREEN_Width - 1;
    if (xs > xe) return;

#ifdef SCREEN_PARTIAL_REFRESH
    int16_t dirty_min = SCREEN_Width;
    int16_t dirty_max = -1;
#endif

    for (int16_t x = xs; x <= xe; x++) {
        SCREEN_Pixel_t val;
        if (mode == SCREEN_Xor && display_ram[y][x] == pixel)
            val = ~pixel;
        else
            val = pixel;
        if (display_ram[y][x] == val) continue;
        display_ram[y][x] = val;
#ifdef SCREEN_PARTIAL_REFRESH
        if (x < dirty_min) dirty_min = x;
        dirty_max = x;
#endif
    }

#ifdef SCREEN_PARTIAL_REFRESH
    if (dirty_max >= dirty_min) {
        mark_dirty_range(y, dirty_min, dirty_max);
    }
#endif
}

void SCREEN_DrawVLine(int16_t x, int16_t y0, int16_t y1, SCREEN_Pixel_t pixel, SCREEN_Mode_t mode) {
    if (x < 0 || x >= SCREEN_Width) return;
    int16_t ys = (y0 < y1) ? y0 : y1;
    int16_t ye = (y0 < y1) ? y1 : y0;
    if (ys < 0) ys = 0;
    if (ye >= SCREEN_Height) ye = SCREEN_Height - 1;
    if (ys > ye) return;

    for (int16_t y = ys; y <= ye; y++) {
        SCREEN_Pixel_t val;
        if (mode == SCREEN_Xor && display_ram[y][x] == pixel)
            val = ~pixel;
        else
            val = pixel;
        if (display_ram[y][x] == val) continue;
        display_ram[y][x] = val;
#ifdef SCREEN_PARTIAL_REFRESH
        mark_dirty(y, x);
#endif
    }
}

/* ========== 图形绘制 ========== */

SCREEN_Event_t SCREEN_DrawPixel(int16_t x, int16_t y, SCREEN_Pixel_t pixel, SCREEN_Mode_t mode) {
    if (x < 0 || x >= SCREEN_Width || y < 0 || y >= SCREEN_Height)
        return SCREEN_OUT;

    if (mode == SCREEN_Xor && SCREEN_GetPixel(x, y) == pixel)
        SCREEN_SetPixel(x, y, ~pixel);
    else
        SCREEN_SetPixel(x, y, pixel);

    return SCREEN_OK;
}

SCREEN_Event_t SCREEN_DrawLine(int16_t x0, int16_t x1, int16_t y0, int16_t y1,
                               SCREEN_Pixel_t pixel, SCREEN_Mode_t mode) {
    int16_t dx = (x1 > x0) ? (x1 - x0) : (x0 - x1);
    int16_t dy = (y1 > y0) ? (y1 - y0) : (y0 - y1);
    int16_t sx = (x0 < x1) ? 1 : -1;
    int16_t sy = (y0 < y1) ? 1 : -1;
    int16_t err = dx - dy;

    while (1) {
        SCREEN_DrawPixel(x0, y0, pixel, mode);

        if (x0 == x1 && y0 == y1) break;

        int16_t e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x0 += sx; }
        if (e2 < dx)  { err += dx; y0 += sy; }
    }

    return SCREEN_OK;
}

SCREEN_Event_t SCREEN_DrawRect(int16_t x0, int16_t x1, int16_t y0, int16_t y1,
                               SCREEN_Pixel_t pixel, SCREEN_Mode_t mode) {
    int16_t xs = (x0 < x1) ? x0 : x1;
    int16_t xe = (x0 < x1) ? x1 : x0;
    int16_t ys = (y0 < y1) ? y0 : y1;
    int16_t ye = (y0 < y1) ? y1 : y0;

    for (int16_t y = ys; y <= ye; y++) {
        SCREEN_DrawHLine(xs, xe, y, pixel, mode);
    }
    return SCREEN_OK;
}

SCREEN_Event_t SCREEN_DrawRectHollow(int16_t x0, int16_t x1, int16_t y0, int16_t y1,
                                     SCREEN_Pixel_t pixel, SCREEN_Mode_t mode) {
    SCREEN_DrawHLine(x0, x1, y0, pixel, mode);
    SCREEN_DrawHLine(x0, x1, y1, pixel, mode);
    SCREEN_DrawVLine(x0, y0 + 1, y1 - 1, pixel, mode);
    SCREEN_DrawVLine(x1, y0 + 1, y1 - 1, pixel, mode);
    return SCREEN_OK;
}

SCREEN_Event_t SCREEN_DrawRoundRect(int16_t x0, int16_t x1, int16_t y0, int16_t y1,
                                     uint8_t radius, SCREEN_Pixel_t pixel, SCREEN_Mode_t mode) {
    int16_t xs = (x0 > x1) ? x1 : x0;
    int16_t xe = (x0 > x1) ? x0 : x1;
    int16_t ys = (y0 > y1) ? y1 : y0;
    int16_t ye = (y0 > y1) ? y0 : y1;

    uint16_t w = (uint16_t)(xe - xs);
    uint16_t h = (uint16_t)(ye - ys);
    if (radius > w / 2) radius = w / 2;
    if (radius > h / 2) radius = h / 2;
    if (radius > SCREEN_MAX_ROUND_RADIUS) radius = SCREEN_MAX_ROUND_RADIUS;  // F-018: 防 left_bound[] 越界

    if (radius == 0)
        return SCREEN_DrawRect(xs, xe, ys, ye, pixel, mode);

    int16_t x = 0;
    int16_t y = radius;
    int16_t d = 3 - 2 * radius;
    int16_t left_bound[SCREEN_MAX_ROUND_RADIUS + 1];

    for (int i = 0; i <= radius; i++)
        left_bound[i] = radius;

    while (x <= y) {
        left_bound[y] = x;
        left_bound[x] = y;
        if (d < 0) {
            d = d + 4 * x + 6;
        } else {
            d = d + 4 * (x - y) + 10;
            y--;
        }
        x++;
    }

    for (int16_t row = ys; row <= ye; row++) {
        int16_t line_start = xs;
        int16_t line_end = xe;

        if (row < ys + radius) {
            int16_t dy = row - (ys + radius);
            int16_t offset = left_bound[abs(dy)];
            line_start = xs + (radius - offset);
            line_end = xe - (radius - offset);
        } else if (row > ye - radius) {
            int16_t dy = row - (ye - radius);
            int16_t offset = left_bound[abs(dy)];
            line_start = xs + (radius - offset);
            line_end = xe - (radius - offset);
        }

        if (line_start <= line_end)
            SCREEN_DrawHLine(line_start, line_end, row, pixel, mode);
    }

    return SCREEN_OK;
}

SCREEN_Event_t SCREEN_DrawRoundRectHollow(int16_t x0, int16_t x1, int16_t y0, int16_t y1,
                                          uint8_t radius, SCREEN_Pixel_t pixel, SCREEN_Mode_t mode) {
    int16_t xs = (x0 > x1) ? x1 : x0;
    int16_t xe = (x0 > x1) ? x0 : x1;
    int16_t ys = (y0 > y1) ? y1 : y0;
    int16_t ye = (y0 > y1) ? y0 : y1;

    uint16_t w = (uint16_t)(xe - xs);
    uint16_t h = (uint16_t)(ye - ys);
    if (radius > w / 2) radius = w / 2;
    if (radius > h / 2) radius = h / 2;
    if (radius > SCREEN_MAX_ROUND_RADIUS) radius = SCREEN_MAX_ROUND_RADIUS;  // F-019: 角点和弧半径必须同步钳位

    if (radius == 0)
        return SCREEN_DrawRectHollow(xs, xe, ys, ye, pixel, mode);

    int16_t tl_x = xs + radius;
    int16_t tl_y = ys + radius;
    int16_t tr_x = xe - radius;
    int16_t tr_y = ys + radius;
    int16_t bl_x = xs + radius;
    int16_t bl_y = ye - radius;
    int16_t br_x = xe - radius;
    int16_t br_y = ye - radius;

    if (w > 2 * radius) {
        SCREEN_DrawHLine(xs + radius + 1, xe - radius - 1, ys, pixel, mode);
        SCREEN_DrawHLine(xs + radius + 1, xe - radius - 1, ye, pixel, mode);
    }
    if (h > 2 * radius) {
        SCREEN_DrawVLine(xs, ys + radius + 1, ye - radius - 1, pixel, mode);
        SCREEN_DrawVLine(xe, ys + radius + 1, ye - radius - 1, pixel, mode);
    }

    SCREEN_DrawArc(tl_x, tl_y, radius, SCREEN_Quarter2, pixel, mode);
    SCREEN_DrawArc(tr_x, tr_y, radius, SCREEN_Quarter1, pixel, mode);
    SCREEN_DrawArc(bl_x, bl_y, radius, SCREEN_Quarter3, pixel, mode);
    SCREEN_DrawArc(br_x, br_y, radius, SCREEN_Quarter4, pixel, mode);

    return SCREEN_OK;
}

SCREEN_Event_t SCREEN_DrawArc(int16_t x0, int16_t y0, uint16_t r, uint8_t quadrant,
                              SCREEN_Pixel_t pixel, SCREEN_Mode_t mode) {
    if (r == 0) return SCREEN_OK;
    if (r > SCREEN_MAX_ROUND_RADIUS) r = SCREEN_MAX_ROUND_RADIUS;

    static uint16_t last_r = 0;
    static int16_t arc_points[SCREEN_MAX_ARC_POINTS][2];
    static uint16_t point_count = 0;

    if (r != last_r) {
        point_count = 0;
        int16_t x = 0;
        int16_t y = (int16_t)r;
        int16_t d = 3 - 2 * r;

        while (x <= y) {
            if (point_count >= SCREEN_MAX_ARC_POINTS) break;

            arc_points[point_count][0] = x;
            arc_points[point_count][1] = y;
            point_count++;

            if (x < y) {
                if (point_count >= SCREEN_MAX_ARC_POINTS) break;
                arc_points[point_count][0] = y;
                arc_points[point_count][1] = x;
                point_count++;
            }

            if (d < 0) {
                d = d + 4 * x + 6;
            } else {
                d = d + 4 * (x - y) + 10;
                y--;
            }
            x++;
        }
        last_r = r;
    }

    for (uint16_t i = 0; i < point_count; i++) {
        int16_t x = arc_points[i][0];
        int16_t y = arc_points[i][1];
        int16_t xt, yt;

        if (quadrant & SCREEN_Quarter1) {
            xt = x0 + x; yt = y0 - y;
            SCREEN_DrawPixel(xt, yt, pixel, mode);
        }
        if (quadrant & SCREEN_Quarter2) {
            xt = x0 - x; yt = y0 - y;
            SCREEN_DrawPixel(xt, yt, pixel, mode);
        }
        if (quadrant & SCREEN_Quarter3) {
            xt = x0 - x; yt = y0 + y;
            SCREEN_DrawPixel(xt, yt, pixel, mode);
        }
        if (quadrant & SCREEN_Quarter4) {
            xt = x0 + x; yt = y0 + y;
            SCREEN_DrawPixel(xt, yt, pixel, mode);
        }
    }

    return SCREEN_OK;
}

static void update_bounds(int16_t x, int16_t y, int16_t *min_x, int16_t *max_x) {
    if (y >= 0 && y < SCREEN_Height) {
        if (x < min_x[y]) min_x[y] = x;
        if (x > max_x[y]) max_x[y] = x;
    }
}

SCREEN_Event_t SCREEN_DrawSector(int16_t x0, int16_t y0, uint16_t r, uint8_t quadrant,
                                 SCREEN_Pixel_t pixel, SCREEN_Mode_t mode) {
    if (r > SCREEN_MAX_ROUND_RADIUS) r = SCREEN_MAX_ROUND_RADIUS;

    int16_t x = 0;
    int16_t y = (int16_t)r;
    int16_t d = 3 - 2 * r;

    /* F-43: 数组尺寸按实际行数 (2*radius+1) 分配, 不用 SCREEN_Height 全占 RAM */
    static int16_t min_x[2 * SCREEN_MAX_ROUND_RADIUS + 1];
    static int16_t max_x[2 * SCREEN_MAX_ROUND_RADIUS + 1];

    int16_t row_lo = y0 - (int16_t)r;
    int16_t row_hi = y0 + (int16_t)r;
    if (row_lo < 0) row_lo = 0;
    if (row_hi >= SCREEN_Height) row_hi = SCREEN_Height - 1;

    for (int16_t i = row_lo; i <= row_hi; i++) {
        min_x[i - row_lo] = SCREEN_Width;
        max_x[i - row_lo] = -1;
    }

    while (x <= y) {
        if (quadrant & SCREEN_Quarter1) {
            update_bounds(x0 + x, y0 - y, min_x, max_x);
            if (x < y) update_bounds(x0 + y, y0 - x, min_x, max_x);
        }
        if (quadrant & SCREEN_Quarter2) {
            update_bounds(x0 - x, y0 - y, min_x, max_x);
            if (x < y) update_bounds(x0 - y, y0 - x, min_x, max_x);
        }
        if (quadrant & SCREEN_Quarter3) {
            update_bounds(x0 - x, y0 + y, min_x, max_x);
            if (x < y) update_bounds(x0 - y, y0 + x, min_x, max_x);
        }
        if (quadrant & SCREEN_Quarter4) {
            update_bounds(x0 + x, y0 + y, min_x, max_x);
            if (x < y) update_bounds(x0 + y, y0 + x, min_x, max_x);
        }

        if (d < 0) {
            d = d + 4 * x + 6;
        } else {
            d = d + 4 * (x - y) + 10;
            y--;
        }
        x++;
    }

    for (int16_t row = row_lo; row <= row_hi; row++) {
        int16_t idx = row - row_lo;
        if (min_x[idx] <= max_x[idx])
            SCREEN_DrawHLine(min_x[idx], max_x[idx], row, pixel, mode);
    }

    return SCREEN_OK;
}

/* ========== 文字绘制 ========== */

SCREEN_Event_t SCREEN_DrawChar(int16_t x, int16_t y, char ch, const struFont_t *font,
                               SCREEN_Pixel_t pixel, SCREEN_Mode_t mode) {
    if (font == NULL || ch < ' ' || ch > '~')
        return SCREEN_CHAR_EXCEED;

    uint8_t idx = (uint8_t)(ch - ' ');
    const uint8_t *char_data = font->font_data;
    if (char_data == NULL) return SCREEN_CHAR_EXCEED;

    char_data += idx * font->height * font->bytes_per_row;

    for (uint8_t row = 0; row < font->height; row++) {
        for (uint8_t col = 0; col < font->width; col++) {
            uint8_t byte_idx = col / 8;
            uint8_t bit_idx = col % 8;
            uint8_t cur_byte = char_data[row * font->bytes_per_row + byte_idx];

            if (cur_byte & (0x01 << bit_idx))
                SCREEN_DrawPixel(x + col, y + row, pixel, mode);
        }
    }

    return SCREEN_OK;
}

SCREEN_Event_t SCREEN_DrawString(int16_t x, int16_t y, const char *str, const struFont_t *font,
                                 SCREEN_Pixel_t pixel, SCREEN_Mode_t mode) {
    int16_t cur_x = x;
    int16_t cur_y = y;

    while (*str != '\0') {
        SCREEN_DrawChar(cur_x, cur_y, *str, font, pixel, mode);
        cur_x += font->width;
        if (cur_x + font->width > SCREEN_Width)
            return SCREEN_OUT;
        str++;
    }
    return SCREEN_OK;
}

SCREEN_Event_t SCREEN_DrawUTFChar(int16_t x, int16_t y, const char *utf8, const struFont_UTF_t *font,
                                   SCREEN_Pixel_t pixel, SCREEN_Mode_t mode) {
    if (utf8 == NULL || font == NULL)
        return SCREEN_PARAM_ERROR;

    const struFont_UTF_data_t *table = font->font_UTF_data;
    const uint8_t *target = (const uint8_t *)utf8;

    /* F-09: 优先用 font->count (编译期已知), 否则按哨兵扫描, 并加安全上限 */
    int table_size = (int)font->count;
    if (table_size <= 0) {
        while (table_size < (int)SCREEN_FONT_UTF_MAX_ENTRIES &&
               !(table[table_size].UTF8_code[0] == 0 &&
                 table[table_size].UTF8_code[1] == 0 &&
                 table[table_size].UTF8_code[2] == 0)) {
            table_size++;
        }
    }

    int lo = 0, hi = table_size - 1;
    int found = -1;
    while (lo <= hi) {
        int mid = lo + (hi - lo) / 2;
        int cmp = 0;
        if (table[mid].UTF8_code[0] != target[0])
            cmp = (table[mid].UTF8_code[0] > target[0]) ? 1 : -1;
        else if (table[mid].UTF8_code[1] != target[1])
            cmp = (table[mid].UTF8_code[1] > target[1]) ? 1 : -1;
        else if (table[mid].UTF8_code[2] != target[2])
            cmp = (table[mid].UTF8_code[2] > target[2]) ? 1 : -1;

        if (cmp == 0) { found = mid; break; }
        if (cmp > 0) hi = mid - 1;
        else lo = mid + 1;
    }

    if (found < 0) return SCREEN_CHAR_EXCEED;

    const uint8_t *char_data = table[found].font_data;

    for (uint8_t row = 0; row < font->height; row++) {
        for (uint8_t col = 0; col < font->width; col++) {
            uint8_t byte_idx = col / 8;
            uint8_t bit_idx = col % 8;
            uint8_t cur_byte = char_data[row * font->bytes_per_row + byte_idx];

            if (cur_byte & (0x01 << bit_idx))
                SCREEN_DrawPixel(x + col, y + row, pixel, mode);
        }
    }

    return SCREEN_OK;
}

static uint8_t UTF8_CharLen(uint8_t c) {
    if ((c & 0x80) == 0x00) return 1;
    if ((c & 0xE0) == 0xC0) return 2;
    if ((c & 0xF0) == 0xE0) return 3;
    if ((c & 0xF8) == 0xF0) return 4;
    return 1;
}

SCREEN_Event_t SCREEN_DrawUTFString(int16_t x, int16_t y, const char *utf8_str,
                                    const struFont_t *ascii, const struFont_UTF_t *hz,
                                    SCREEN_Pixel_t pixel, SCREEN_Mode_t mode) {
    if (utf8_str == NULL || ascii == NULL || hz == NULL)
        return SCREEN_PARAM_ERROR;

    int16_t cur_x = x;
    int16_t cur_y = y;
    const char *p = utf8_str;

    while (*p != '\0') {
        uint8_t char_len = UTF8_CharLen((uint8_t)*p);
        uint16_t char_w = (char_len == 1) ? ascii->width : hz->width;

        if (cur_x + char_w > SCREEN_Width)
            break;

        if (char_len == 1) {
            if (*p >= ' ' && *p <= '~')
                SCREEN_DrawChar(cur_x, cur_y, *p, ascii, pixel, mode);
            cur_x += ascii->width;
            p++;
        } else if (char_len == 3) {
            /* 3 字节 UTF-8 (基本汉字/日韩): 命中与否都前进 hz->width */
            SCREEN_DrawUTFChar(cur_x, cur_y, p, hz, pixel, mode);
            cur_x += hz->width;
            p += 3;
        } else if (char_len == 2) {
            /* 2 字节 UTF-8 (Latin-1 扩展): 半宽前进, 跳过字节不绘 */
            cur_x += hz->width / 2;
            p += 2;
        } else {
            /* 4 字节 UTF-8 (Emoji/扩展 CJK): 全宽前进 */
            cur_x += hz->width;
            p += char_len;
        }
    }

    return SCREEN_OK;
}

/* ========== 图片绘制 ========== */

SCREEN_Event_t SCREEN_DrawImage(int16_t x, int16_t y, uint16_t w, uint16_t h,
                                 const uint8_t *img, SCREEN_Pixel_t pixel, SCREEN_Mode_t mode) {
    if (img == NULL || w == 0 || h == 0)
        return SCREEN_PARAM_ERROR;

    uint16_t bytes_per_line = (w + 7) / 8;

    for (uint16_t row = 0; row < h; row++) {
        int16_t cur_y = (int16_t)(y + row);
        if (cur_y < 0 || cur_y >= SCREEN_Height) continue;

        for (uint16_t col = 0; col < w; col++) {
            int16_t cur_x = (int16_t)(x + col);
            if (cur_x < 0 || cur_x >= SCREEN_Width) continue;

            uint16_t byte_idx = row * bytes_per_line + (col / 8);
            uint8_t bit_idx = (uint8_t)(7 - (col % 8));
            uint8_t cur_byte = img[byte_idx];

            if (cur_byte & (0x01 << bit_idx))
                SCREEN_DrawPixel(cur_x, cur_y, pixel, mode);
        }
    }
    return SCREEN_OK;
}

SCREEN_Event_t SCREEN_DrawRGBImage(int16_t x, int16_t y, uint16_t w, uint16_t h,
                                   const uint8_t *img) {
    if (w == 0 || h == 0 || img == NULL)
        return SCREEN_OK;

    for (uint16_t row = 0; row < h; row++) {
        for (uint16_t col = 0; col < w; col++) {
            uint16_t idx = (row * w + col) * 2;
            SCREEN_Pixel_t pixel = (SCREEN_Pixel_t)(img[idx + 1] << 8 | img[idx]);
            SCREEN_DrawPixel((int16_t)(x + col), (int16_t)(y + row), pixel, SCREEN_Nor);
        }
    }

    return SCREEN_OK;
}