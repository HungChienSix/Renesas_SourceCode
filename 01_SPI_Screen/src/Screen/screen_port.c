/* V1.0 Screen Porting Layer - Renesas RA Implementation */

#include "screen_port.h"

/* SPI 发送完成标志 */
volatile bool g_screen_port_spi_transfer_complete_flag = false;

/* SPI 发送函数 */
static bool SCREEN_PORT_SpiTransfer(void const * p_src, uint32_t length, uint32_t timeout_us) {
    g_screen_port_spi_transfer_complete_flag = false;

    fsp_err_t err = R_SCI_SPI_Write(&SCREEN_SPI_CTRL, p_src, length, SPI_BIT_WIDTH_8_BITS);
    if (err != FSP_SUCCESS) {
        return false;
    }

    /* 先紧密自旋，快速捕获短传输完成 */
    uint32_t spins = 0;
    while (!g_screen_port_spi_transfer_complete_flag) {
        if (++spins > 100) {
            if (timeout_us == 0) break;
            timeout_us--;
            R_BSP_SoftwareDelay(1U, BSP_DELAY_UNITS_MICROSECONDS);
        }
    }

    if (!g_screen_port_spi_transfer_complete_flag) {
        return false;
    }
    return true;
}

/* GPIO 写函数 */
static void SCREEN_PORT_GpioWrite(uint32_t pin, uint8_t level) {
    bsp_io_level_t bsp_level = (level == 0) ? BSP_IO_LEVEL_LOW : BSP_IO_LEVEL_HIGH;
    R_IOPORT_PinWrite(&g_ioport_ctrl, pin, bsp_level);
}

/* 毫秒延时函数 */
static void SCREEN_PORT_DelayMs(uint32_t delay, uint8_t units) {
    (void)units;
    R_BSP_SoftwareDelay(delay, BSP_DELAY_UNITS_MILLISECONDS);
}

/* 微秒延时函数 */
static void SCREEN_PORT_DelayUs(uint32_t delay, uint8_t units) {
    (void)units;
    R_BSP_SoftwareDelay(delay, BSP_DELAY_UNITS_MICROSECONDS);
}

/* 屏幕平台接口 - Renesas RA */
static const screen_port_t g_screen_port = {
    .spi_transfer = SCREEN_PORT_SpiTransfer,
    .gpio_write   = SCREEN_PORT_GpioWrite,
    .delay_ms     = SCREEN_PORT_DelayMs,
    .delay_us     = SCREEN_PORT_DelayUs,
};

const screen_port_t * screen_port_get(void) {
    return &g_screen_port;
}

/* SPI 中断回调函数 - 必须保持原名以匹配FSP配置 */
void sci_spi_callback(spi_callback_args_t *p_args) {
    switch (p_args->event) {
        case SPI_EVENT_TRANSFER_COMPLETE:
            g_screen_port_spi_transfer_complete_flag = true;
            break;
        default:
            break;
    }
}

/* 初始化屏幕硬件平台 */
int screen_port_init(void) {
    fsp_err_t err = R_SCI_SPI_Open(&SCREEN_SPI_CTRL, &SCREEN_SPI_CFG);
    return (err == FSP_SUCCESS) ? 0 : (int)err;
}