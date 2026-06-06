/* V1.0 ST7789 Screen Controller Driver */

#include "st7789.h"
#include "hal_port/screen_port.h"

/* 通过平台接口获取屏幕硬件操作函数 */
static const screen_port_t * ST7789_PortGet(void) {
    return screen_port_get();
}

/* ========== 硬件底层操作 ========== */

void ST7789_Reset(void) {
    ST7789_PortGet()->gpio_write(SCREEN_RES_Pin, 0);
    ST7789_PortGet()->delay_ms(100);
    ST7789_PortGet()->gpio_write(SCREEN_RES_Pin, 1);
    ST7789_PortGet()->delay_ms(100);
}

void ST7789_WriteCmd(uint8_t cmd) {
    ST7789_PortGet()->gpio_write(SCREEN_DC_Pin, 0);
    ST7789_PortGet()->gpio_write(SCREEN_CS_Pin, 0);
    ST7789_PortGet()->spi_transfer(&cmd, 1, 100000);
    ST7789_PortGet()->gpio_write(SCREEN_CS_Pin, 1);
}

void ST7789_WriteByte(uint8_t data) {
    ST7789_PortGet()->gpio_write(SCREEN_DC_Pin, 1);
    ST7789_PortGet()->gpio_write(SCREEN_CS_Pin, 0);
    ST7789_PortGet()->spi_transfer(&data, 1, 100000);
    ST7789_PortGet()->gpio_write(SCREEN_CS_Pin, 1);
}

int ST7789_WriteData(uint8_t *data, size_t data_size, uint32_t timeout) {
    ST7789_PortGet()->gpio_write(SCREEN_DC_Pin, 1);
    ST7789_PortGet()->gpio_write(SCREEN_CS_Pin, 0);
    int ret = ST7789_PortGet()->spi_transfer(data, data_size, timeout);
    ST7789_PortGet()->gpio_write(SCREEN_CS_Pin, 1);
    return ret;
}

/* ========== ST7789 控制器初始化 ========== */

static void ST7789_InitSequence(void) {
    /* Sleep out */
    ST7789_WriteCmd(ST7789_SLPOUT);
    ST7789_PortGet()->delay_ms(120);

    /* MADCTL - 屏幕方向 (横屏) */
    ST7789_WriteCmd(ST7789_MADCTL);
    ST7789_WriteByte(ST7789_MADCTL_Parameter);

    /* Pixel format */
    ST7789_WriteCmd(ST7789_COLMOD);
    ST7789_WriteByte(0x05);

    /* Frame rate */
    ST7789_WriteCmd(ST7789_FRMCTR1);
    ST7789_WriteByte(0x05);
    ST7789_WriteByte(0x05);
    ST7789_WriteByte(0x00);
    ST7789_WriteByte(0x33);
    ST7789_WriteByte(0x33);

    /* Power control */
    ST7789_WriteCmd(ST7789_PWCTR1);
    ST7789_WriteByte(0x35);

    ST7789_WriteCmd(ST7789_PWCTR2);
    ST7789_WriteByte(0x21);

    ST7789_WriteCmd(ST7789_VMCTR);
    ST7789_WriteByte(0x35);

    /* Power control sequences */
    ST7789_WriteCmd(ST7789_PWCTR3);
    ST7789_WriteByte(0x2C);

    ST7789_WriteCmd(ST7789_PWCTR4);
    ST7789_WriteByte(0x01);

    ST7789_WriteCmd(ST7789_PWCTR5);
    ST7789_WriteByte(0x0B);

    ST7789_WriteCmd(ST7789_PWCTR6);
    ST7789_WriteByte(0x01);

    /* Gamma correction */
    ST7789_WriteCmd(ST7789_GAMCTRP);
    ST7789_WriteByte(0xD0);
    ST7789_WriteByte(0x04);
    ST7789_WriteByte(0x08);
    ST7789_WriteByte(0x0A);
    ST7789_WriteByte(0x09);
    ST7789_WriteByte(0x05);
    ST7789_WriteByte(0x2D);
    ST7789_WriteByte(0x43);
    ST7789_WriteByte(0x49);
    ST7789_WriteByte(0x09);
    ST7789_WriteByte(0x16);
    ST7789_WriteByte(0x15);
    ST7789_WriteByte(0x26);
    ST7789_WriteByte(0x2B);

    ST7789_WriteCmd(ST7789_GAMCTRN);
    ST7789_WriteByte(0xD0);
    ST7789_WriteByte(0x03);
    ST7789_WriteByte(0x09);
    ST7789_WriteByte(0x0A);
    ST7789_WriteByte(0x0A);
    ST7789_WriteByte(0x06);
    ST7789_WriteByte(0x2E);
    ST7789_WriteByte(0x44);
    ST7789_WriteByte(0x40);
    ST7789_WriteByte(0x3A);
    ST7789_WriteByte(0x15);
    ST7789_WriteByte(0x15);
    ST7789_WriteByte(0x26);
    ST7789_WriteByte(0x2A);

    /* Display on */
    ST7789_WriteCmd(ST7789_DISPON);
}

/* 初始化屏幕硬件平台（SPI初始化） */
fsp_err_t ST7789_Hardware_Init(void) {
    return (fsp_err_t)screen_port_init();
}

fsp_err_t ST7789_Init(void) {
    ST7789_PortGet()->gpio_write(SCREEN_BLK_Pin, 1);
    ST7789_Reset();
    ST7789_InitSequence();
    return FSP_SUCCESS;
}

/* ========== 地址窗口设置 ========== */

void ST7789_SetAddressWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    uint8_t data[4] = {0x00};

    x0 += ST7789_XSTART;
    x1 += ST7789_XSTART;
    y0 += ST7789_YSTART;
    y1 += ST7789_YSTART;

    ST7789_WriteCmd(ST7789_CASET);
    data[0] = (uint8_t)(x0 >> 8);
    data[1] = (uint8_t)(x0 & 0xFF);
    data[2] = (uint8_t)(x1 >> 8);
    data[3] = (uint8_t)(x1 & 0xFF);
    (void)ST7789_WriteData(data, sizeof(data), 100000);  // 初始化期 SPI 失败由 SCREEN_Init 后续处理

    ST7789_WriteCmd(ST7789_RASET);
    data[0] = (uint8_t)(y0 >> 8);
    data[1] = (uint8_t)(y0 & 0xFF);
    data[2] = (uint8_t)(y1 >> 8);
    data[3] = (uint8_t)(y1 & 0xFF);
    (void)ST7789_WriteData(data, sizeof(data), 100000);
}