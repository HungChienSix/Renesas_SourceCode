/* V1.0 ST7735 Screen Controller Driver */

#include "st7735.h"
#include "hal_port/screen_port.h"

/* 通过平台接口获取屏幕硬件操作函数 */
static const screen_port_t * ST7735_PortGet(void) {
    return screen_port_get();
}

/* ========== 硬件底层操作 ========== */

void ST7735_Reset(void) {
    ST7735_PortGet()->gpio_write(SCREEN_RES_Pin, 0);
    ST7735_PortGet()->delay_ms(100);
    ST7735_PortGet()->gpio_write(SCREEN_RES_Pin, 1);
    ST7735_PortGet()->delay_ms(100);
}

void ST7735_WriteCmd(uint8_t cmd) {
    ST7735_PortGet()->gpio_write(SCREEN_DC_Pin, 0);
    ST7735_PortGet()->gpio_write(SCREEN_CS_Pin, 0);
    ST7735_PortGet()->spi_transfer(&cmd, 1, 100000);
    ST7735_PortGet()->gpio_write(SCREEN_CS_Pin, 1);
}

void ST7735_WriteByte(uint8_t data) {
    ST7735_PortGet()->gpio_write(SCREEN_DC_Pin, 1);
    ST7735_PortGet()->gpio_write(SCREEN_CS_Pin, 0);
    ST7735_PortGet()->spi_transfer(&data, 1, 100000);
    ST7735_PortGet()->gpio_write(SCREEN_CS_Pin, 1);
}

int ST7735_WriteData(uint8_t *data, size_t data_size, uint32_t timeout) {
    ST7735_PortGet()->gpio_write(SCREEN_DC_Pin, 1);
    ST7735_PortGet()->gpio_write(SCREEN_CS_Pin, 0);
    int ret = ST7735_PortGet()->spi_transfer(data, data_size, timeout);
    ST7735_PortGet()->gpio_write(SCREEN_CS_Pin, 1);
    return ret;
}

/* ========== ST7735 控制器初始化 ========== */

static void ST7735_InitSequence(void) {
    /* Sleep out */
    ST7735_WriteCmd(ST7735_SLPOUT);
    ST7735_PortGet()->delay_ms(120);

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
    ST7735_PortGet()->gpio_write(SCREEN_BLK_Pin, 1);
    ST7735_Reset();
    ST7735_InitSequence();
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
    (void)ST7735_WriteData(data, sizeof(data), 100000);  // 初始化期 SPI 失败由 SCREEN_Init 后续处理

    ST7735_WriteCmd(ST7735_RASET);
    data[0] = (uint8_t)(y0 >> 8);
    data[1] = (uint8_t)(y0 & 0xFF);
    data[2] = (uint8_t)(y1 >> 8);
    data[3] = (uint8_t)(y1 & 0xFF);
    (void)ST7735_WriteData(data, sizeof(data), 100000);
}