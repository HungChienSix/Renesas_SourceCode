/* V1.0 ST7735 Screen Controller Driver */

#include "st7735.h"
#include "sys_time/sys_time.h"
#include "bsp_api.h"

/* 帧缓冲区：128x128 像素, 16位色 (RGB565) */
static uint16_t display_ram[ST7735_HEIGHT][ST7735_WIDTH] = {0x0000};

#ifdef ST7735_PARTIAL_REFRESH
/* 行级脏标记 */
static bool dirty_row[ST7735_HEIGHT] = {false};
/* 列范围脏标记：每行记录脏列的 [min, max] 范围 */
static uint16_t dirty_col_min[ST7735_HEIGHT];
static uint16_t dirty_col_max[ST7735_HEIGHT];
/* 发送缓冲区：最大单行 128*2 字节 */
static uint8_t batch_buf[ST7735_WIDTH * 2];

/* 标记单列为脏 */
static inline void mark_dirty(int16_t y, int16_t x) {
    if (!dirty_row[y]) {
        dirty_col_min[y] = x;
        dirty_col_max[y] = x;
        dirty_row[y] = true;
    } else {
        if (x < dirty_col_min[y]) dirty_col_min[y] = x;
        if (x > dirty_col_max[y]) dirty_col_max[y] = x;
    }
}

/* 标记列范围为脏 [x0, x1] */
static inline void mark_dirty_range(int16_t y, int16_t x0, int16_t x1) {
    if (!dirty_row[y]) {
        dirty_col_min[y] = x0;
        dirty_col_max[y] = x1;
        dirty_row[y] = true;
    } else {
        if (x0 < dirty_col_min[y]) dirty_col_min[y] = x0;
        if (x1 > dirty_col_max[y]) dirty_col_max[y] = x1;
    }
}
#endif

/* 通过平台接口获取屏幕硬件操作函数 */
static const screen_port_t * ST7735_PortGet(void) {
    return screen_port_get();
}

/* ========== 硬件底层操作 ========== */

void ST7735_Reset(void) {
    ST7735_PortGet()->gpio_write(TFT_RES_Pin, 0);
    ST7735_PortGet()->delay_ms(100, 0);
    ST7735_PortGet()->gpio_write(TFT_RES_Pin, 1);
    ST7735_PortGet()->delay_ms(100, 0);
}

void ST7735_WriteCmd(uint8_t cmd) {
    ST7735_PortGet()->gpio_write(TFT_DC_Pin, 0);
    ST7735_PortGet()->gpio_write(TFT_CS_Pin, 0);
    ST7735_PortGet()->spi_transfer(&cmd, 1, 100000);
    ST7735_PortGet()->gpio_write(TFT_CS_Pin, 1);
}

void ST7735_WriteByte(uint8_t data) {
    ST7735_PortGet()->gpio_write(TFT_DC_Pin, 1);
    ST7735_PortGet()->gpio_write(TFT_CS_Pin, 0);
    ST7735_PortGet()->spi_transfer(&data, 1, 100000);
    ST7735_PortGet()->gpio_write(TFT_CS_Pin, 1);
}

void ST7735_WriteData(uint8_t *data, size_t data_size) {
    ST7735_PortGet()->gpio_write(TFT_DC_Pin, 1);
    ST7735_PortGet()->gpio_write(TFT_CS_Pin, 0);
    ST7735_PortGet()->spi_transfer(data, data_size, 100000);
    ST7735_PortGet()->gpio_write(TFT_CS_Pin, 1);
}

/* ========== ST7735 控制器初始化 ========== */

static void ST7735_InitSequence(void) {
    /* Sleep out */
    ST7735_WriteCmd(ST7735_SLPOUT);
    ST7735_PortGet()->delay_ms(120, 0);

    /* Frame rate */
    ST7735_WriteCmd(ST7735_FRMCTR1);
    ST7735_WriteByte(0x01);
    ST7735_WriteByte(0x2C);
    ST7735_WriteByte(0x2D);
    ST7735_WriteCmd(ST7735_FRMCTR2);
    ST7735_WriteByte(0x01);
    ST7735_WriteByte(0x2C);
    ST7735_WriteByte(0x2D);
    ST7735_WriteCmd(ST7735_FRMCTR3);
    ST7735_WriteByte(0x01);
    ST7735_WriteByte(0x2C);
    ST7735_WriteByte(0x2D);
    ST7735_WriteByte(0x01);
    ST7735_WriteByte(0x2C);
    ST7735_WriteByte(0x2D);
    ST7735_WriteCmd(ST7735_INVCTR);
    ST7735_WriteByte(0x07);

    /* Power control */
    ST7735_WriteCmd(ST7735_PWCTR1);
    ST7735_WriteByte(0xA8);
    ST7735_WriteByte(0x08);
    ST7735_WriteByte(0x84);
    ST7735_WriteCmd(ST7735_PWCTR2);
    ST7735_WriteByte(0xC4);
    ST7735_WriteCmd(ST7735_PWCTR3);
    ST7735_WriteByte(0x0D);
    ST7735_WriteByte(0x00);
    ST7735_WriteCmd(ST7735_PWCTR4);
    ST7735_WriteByte(0x8D);
    ST7735_WriteByte(0x6A);
    ST7735_WriteCmd(ST7735_PWCTR5);
    ST7735_WriteByte(0x8D);
    ST7735_WriteByte(0xEE);
    ST7735_WriteCmd(ST7735_VMCTR);
    ST7735_WriteByte(0x05);

    /* Gamma correction */
    ST7735_WriteCmd(ST7735_GAMCTRP);
    ST7735_WriteByte(0x02);
    ST7735_WriteByte(0x1C);
    ST7735_WriteByte(0x07);
    ST7735_WriteByte(0x12);
    ST7735_WriteByte(0x37);
    ST7735_WriteByte(0x32);
    ST7735_WriteByte(0x29);
    ST7735_WriteByte(0x2D);
    ST7735_WriteByte(0x29);
    ST7735_WriteByte(0x25);
    ST7735_WriteByte(0x2B);
    ST7735_WriteByte(0x39);
    ST7735_WriteByte(0x00);
    ST7735_WriteByte(0x01);
    ST7735_WriteByte(0x03);
    ST7735_WriteByte(0x10);
    ST7735_WriteCmd(ST7735_GAMCTRN);
    ST7735_WriteByte(0x03);
    ST7735_WriteByte(0x1D);
    ST7735_WriteByte(0x07);
    ST7735_WriteByte(0x06);
    ST7735_WriteByte(0x2E);
    ST7735_WriteByte(0x2C);
    ST7735_WriteByte(0x29);
    ST7735_WriteByte(0x2D);
    ST7735_WriteByte(0x2E);
    ST7735_WriteByte(0x2E);
    ST7735_WriteByte(0x37);
    ST7735_WriteByte(0x3F);
    ST7735_WriteByte(0x00);
    ST7735_WriteByte(0x00);
    ST7735_WriteByte(0x02);
    ST7735_WriteByte(0x10);

    /* Display settings */
    ST7735_WriteCmd(ST7735_INVOFF);
    ST7735_WriteCmd(ST7735_COLMOD);
    ST7735_WriteByte(0x05);

    /* Address mode */
    ST7735_WriteCmd(ST7735_CASET);
    ST7735_WriteByte(0x00);
    ST7735_WriteByte(0x00);
    ST7735_WriteByte((ST7735_WIDTH - 1) >> 8);
    ST7735_WriteByte(ST7735_WIDTH - 1);
    ST7735_WriteCmd(ST7735_RASET);
    ST7735_WriteByte(0x00);
    ST7735_WriteByte(0x00);
    ST7735_WriteByte((ST7735_HEIGHT - 1) >> 8);
    ST7735_WriteByte(ST7735_HEIGHT - 1);
    ST7735_WriteCmd(ST7735_MADCTL);
    ST7735_WriteByte(ST7735_MADCTL_Parameter);

    /* Display on */
    ST7735_WriteCmd(ST7735_DISPON);
}

/* 初始化屏幕硬件平台（SPI初始化） */
fsp_err_t ST7735_Hardware_Init(void) {
    return (fsp_err_t)screen_port_init();
}

fsp_err_t ST7735_Init(void) {
    ST7735_Reset();
    ST7735_InitSequence();
    ST7735_FillScreen(SCREEN_BLACK);
    ST7735_RefreshScreen_Force();

    return FSP_SUCCESS;
}

/* ========== 地址窗口设置 ========== */

void ST7735_SetAddressWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    uint8_t data[4] = {0x00};

    x0 += ST7735_XSTART;
    x1 += ST7735_XSTART;
    y0 += ST7735_YSTART;
    y1 += ST7735_YSTART;

    ST7735_WriteCmd(ST7735_CASET);
    data[0] = (uint8_t)(x0 >> 8);
    data[1] = (uint8_t)(x0 & 0xFF);
    data[2] = (uint8_t)(x1 >> 8);
    data[3] = (uint8_t)(x1 & 0xFF);
    ST7735_WriteData(data, sizeof(data));

    ST7735_WriteCmd(ST7735_RASET);
    data[0] = (uint8_t)(y0 >> 8);
    data[1] = (uint8_t)(y0 & 0xFF);
    data[2] = (uint8_t)(y1 >> 8);
    data[3] = (uint8_t)(y1 & 0xFF);
    ST7735_WriteData(data, sizeof(data));
}

/* ========== 绘图函数 ========== */

ST7735_Pixel_t ST7735_ReadPixel(int16_t x, int16_t y) {
    return display_ram[y][x];
}

void ST7735_DrawPixel(int16_t x, int16_t y, ST7735_Pixel_t Pixel_Set) {
    if ((x < 0) || (y < 0) || (x > ST7735_WIDTH - 1) || (y > ST7735_HEIGHT - 1))
        return;
    if (ST7735_ReadPixel(x, y) == Pixel_Set)
        return;
    display_ram[y][x] = Pixel_Set;
#ifdef ST7735_PARTIAL_REFRESH
    mark_dirty(y, x);
#endif
}

void ST7735_DrawHorLine(int16_t x0, int16_t x1, int16_t y, ST7735_Pixel_t Pixel_Set, SCREEN_Mode_t type) {
    /* 边界裁剪 */
    if (y < 0 || y >= ST7735_HEIGHT) return;
    int16_t x_start = (x0 < x1) ? x0 : x1;
    int16_t x_end   = (x0 < x1) ? x1 : x0;
    if (x_start < 0) x_start = 0;
    if (x_end >= ST7735_WIDTH) x_end = ST7735_WIDTH - 1;
    if (x_start > x_end) return;

#ifdef ST7735_PARTIAL_REFRESH
    int16_t dirty_min = ST7735_WIDTH;
    int16_t dirty_max = -1;
#endif

    for (int16_t x = x_start; x <= x_end; x++) {
        ST7735_Pixel_t val;
        if (type == SCREEN_Xor && display_ram[y][x] == Pixel_Set)
            val = ~Pixel_Set;
        else
            val = Pixel_Set;
        if (display_ram[y][x] == val) continue;
        display_ram[y][x] = val;
#ifdef ST7735_PARTIAL_REFRESH
        if (x < dirty_min) dirty_min = x;
        dirty_max = x;
#endif
    }

#ifdef ST7735_PARTIAL_REFRESH
    if (dirty_max >= dirty_min) {
        mark_dirty_range(y, dirty_min, dirty_max);
    }
#endif
}

void ST7735_DrawVerLine(int16_t x, int16_t y0, int16_t y1, ST7735_Pixel_t Pixel_Set, SCREEN_Mode_t type) {
    /* 边界裁剪 */
    if (x < 0 || x >= ST7735_WIDTH) return;
    int16_t y_start = (y0 < y1) ? y0 : y1;
    int16_t y_end   = (y0 < y1) ? y1 : y0;
    if (y_start < 0) y_start = 0;
    if (y_end >= ST7735_HEIGHT) y_end = ST7735_HEIGHT - 1;
    if (y_start > y_end) return;

    for (int16_t y = y_start; y <= y_end; y++) {
        ST7735_Pixel_t val;
        if (type == SCREEN_Xor && display_ram[y][x] == Pixel_Set)
            val = ~Pixel_Set;
        else
            val = Pixel_Set;

        if (display_ram[y][x] == val) continue;
        display_ram[y][x] = val;
#ifdef ST7735_PARTIAL_REFRESH
        mark_dirty(y, x);
#endif
    }
}

void ST7735_FillScreen(ST7735_Pixel_t Pixel_Set) {
#ifdef ST7735_PARTIAL_REFRESH
    for (int16_t y = 0; y < ST7735_HEIGHT; y++) {
        int16_t c0 = -1;
        /* 从前向后找第一个不同的像素 */
        for (int16_t x = 0; x < ST7735_WIDTH; x++) {
            if (display_ram[y][x] != Pixel_Set) { c0 = x; break; }
        }
        if (c0 < 0) continue; /* 该行无变化，跳过 */

        /* 从后向前找最后一个不同的像素 */
        int16_t c1 = c0;
        for (int16_t x = ST7735_WIDTH - 1; x > c0; x--) {
            if (display_ram[y][x] != Pixel_Set) { c1 = x; break; }
        }

        /* 只填充变化的范围 */
        for (int16_t x = c0; x <= c1; x++)
            display_ram[y][x] = Pixel_Set;
        mark_dirty_range(y, c0, c1);
    }
#else
    /* RGB565: 高低字节相同时（如0x0000黑、0xFFFF白）可用 memset 加速 */
    uint8_t hi = (uint8_t)(Pixel_Set >> 8);
    uint8_t lo = (uint8_t)(Pixel_Set & 0xFF);
    if (hi == lo) {
        memset(display_ram, hi, sizeof(display_ram));
    } else {
        for (uint32_t i = 0; i < ST7735_HEIGHT * ST7735_WIDTH; i++) {
            ((ST7735_Pixel_t *)display_ram)[i] = Pixel_Set;
        }
    }
#endif
}

void ST7735_RefreshScreen_Force(void) {
    ST7735_SetAddressWindow(0, 0, ST7735_WIDTH - 1, ST7735_HEIGHT - 1);
    ST7735_WriteCmd(ST7735_RAMWR);

    static uint8_t buff[ST7735_WIDTH * 2];  /* 单行缓冲区 */

    for (uint16_t h = 0; h < ST7735_HEIGHT; h++) {
        for (uint16_t w = 0; w < ST7735_WIDTH; w++) {
            buff[w * 2] = (uint8_t)(display_ram[h][w] >> 8);
            buff[w * 2 + 1] = (uint8_t)(display_ram[h][w] & 0xFF);
        }
        ST7735_WriteData(buff, ST7735_WIDTH * 2);
    }
}

uint32_t ST7735_RefreshScreen(void) {
    uint32_t start_time = SysTime_Get_us();

#ifdef ST7735_PARTIAL_REFRESH
    for (int16_t row = 0; row < ST7735_HEIGHT; row++) {
        if (!dirty_row[row]) continue;

        uint8_t c0 = dirty_col_min[row];
        uint8_t c1 = dirty_col_max[row];

        ST7735_SetAddressWindow(c0, row, c1, row);
        ST7735_WriteCmd(ST7735_RAMWR);

        int buf_idx = 0;
        for (uint16_t w = c0; w <= c1; w++) {
            batch_buf[buf_idx++] = (uint8_t)(display_ram[row][w] >> 8);
            batch_buf[buf_idx++] = (uint8_t)(display_ram[row][w] & 0xFF);
        }
        ST7735_WriteData(batch_buf, buf_idx);

        dirty_row[row] = false;
    }
#else
    ST7735_RefreshScreen_Force();
#endif

    uint32_t end_time = SysTime_Get_us();
    return SysTime_Elapsed_us(start_time, end_time) / 1000;
}