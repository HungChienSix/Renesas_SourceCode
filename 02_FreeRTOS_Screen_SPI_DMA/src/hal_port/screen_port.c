/* V1.0 Screen Porting Layer - Renesas RA Implementation
 *
 * 与 03_FreeRTOS_SHT40_I2C/src/hal_port/i2c_port.c 的区别:
 *   - 位置: 本项目 src/hal_port/  (i2c_port.c 在 03 项目的 src/hal_port/)
 *   - 同步: 保留本项目 FreeRTOS 风格 (binary semaphore + ISR give)
 *           替代 i2c_port.c 的紧密自旋 (BSP_SoftwareDelay + 标志轮询)
 *   - 命名: 函数/类型沿用 screen_* 前缀, 与 i2c_port.c 的 i2c_* 对仗
 */

#include "screen_port.h"
#include "FreeRTOS.h"
#include "semphr.h"

/* SPI 发送完成标志 (与 FSP 配置的 spi_callback 共享) */
volatile bool g_screen_port_spi_transfer_complete = false;

/* binary semaphore: ISR → 任务同步, 替代自旋等待
 * 任务侧 R_SPI_Write 之后 xSemaphoreTake(..., timeout);
 * ISR 侧 spi_callback 完成时 xSemaphoreGiveFromISR;
 * 关键: 每次发送前必须先 xSemaphoreTake(..., 0) 清空残留 give
 * 注意: 本项目 configSUPPORT_DYNAMIC_ALLOCATION=0, 只能用 Static 版 API */
static StaticSemaphore_t s_spi_done_sem_buf;
static SemaphoreHandle_t s_spi_done_sem = NULL;

/* SPI 发送 (带超时阻塞) - 0 = 非阻塞, 否则按 timeout_us 阻塞
 * 实现: 先清空信号量, 启动 DMA 传输, 阻塞等待 ISR 通知 */
static int screen_hal_spi_transfer(void const * p_src, uint32_t length, uint32_t timeout_us) {
    /* 清空残留 give (上一次的回调可能还没被 take 就来了第二次) */
    if (s_spi_done_sem != NULL) {
        while (xSemaphoreTake(s_spi_done_sem, 0) == pdTRUE) { }
    }
    g_screen_port_spi_transfer_complete = false;

    fsp_err_t err = R_SPI_Write(&SCREEN_SPI_CTRL, p_src, length, SPI_BIT_WIDTH_8_BITS);
    if (err != FSP_SUCCESS) {
        return (int)err;
    }

    if (s_spi_done_sem == NULL) {
        /* 信号量未初始化: 退化到原自旋实现 (启动期或异常路径) */
        uint32_t spins = 0;
        while (!g_screen_port_spi_transfer_complete) {
            if (++spins > 100U) {
                if (timeout_us == 0U) break;
                R_BSP_SoftwareDelay(1U, BSP_DELAY_UNITS_MICROSECONDS);
                if (timeout_us != UINT32_MAX) timeout_us--;
            }
        }
        return g_screen_port_spi_transfer_complete ? 0 : (int)FSP_ERR_TIMEOUT;
    }

    /* 阻塞等待: 0 = 非阻塞, UINT32_MAX = 永久, 否则 us → ms 向上取整 */
    TickType_t timeout_ticks;
    if (timeout_us == 0U) {
        timeout_ticks = 0;
    } else if (timeout_us == UINT32_MAX) {
        timeout_ticks = portMAX_DELAY;
    } else {
        uint32_t ms = (timeout_us + 999U) / 1000U;
        timeout_ticks = pdMS_TO_TICKS(ms);
        if (timeout_ticks == 0) timeout_ticks = 1;
    }

    if (xSemaphoreTake(s_spi_done_sem, timeout_ticks) != pdTRUE) {
        return (int)FSP_ERR_TIMEOUT;
    }
    return 0;
}

/* GPIO 写电平 (0/1) */
static void screen_hal_gpio_write(uint32_t pin, uint8_t level) {
    bsp_io_level_t bsp_level = (level == 0U) ? BSP_IO_LEVEL_LOW : BSP_IO_LEVEL_HIGH;
    R_IOPORT_PinWrite(&g_ioport_ctrl, pin, bsp_level);
}

/* 毫秒延时 */
static void screen_hal_delay_ms(uint32_t ms) {
    R_BSP_SoftwareDelay(ms, BSP_DELAY_UNITS_MILLISECONDS);
}

/* 微秒延时 */
static void screen_hal_delay_us(uint32_t us) {
    R_BSP_SoftwareDelay(us, BSP_DELAY_UNITS_MICROSECONDS);
}

/* 屏幕平台接口实例 - 与 i2c_port.c 的 g_i2c_port 对仗 */
static const screen_port_t g_screen_port = {
    .spi_transfer = screen_hal_spi_transfer,
    .gpio_write   = screen_hal_gpio_write,
    .delay_ms     = screen_hal_delay_ms,
    .delay_us     = screen_hal_delay_us,
};

const screen_port_t * screen_port_get(void) {
    return &g_screen_port;
}

/* SPI 中断回调 (名字必须与 FSP 配置一致: spi_callback)
 * 在 ISR 中 give 信号量, 唤醒等待的发送任务 */
void spi_callback(spi_callback_args_t *p_args) {
    if (p_args->event == SPI_EVENT_TRANSFER_COMPLETE) {
        g_screen_port_spi_transfer_complete = true;
        if (s_spi_done_sem != NULL) {
            BaseType_t xHigherPriorityTaskWoken = pdFALSE;
            xSemaphoreGiveFromISR(s_spi_done_sem, &xHigherPriorityTaskWoken);
            portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
        }
    }
}

/* 初始化屏幕硬件平台 (打开 SPI 外设 + 创建信号量) */
int screen_port_init(void) {
    fsp_err_t err = R_SPI_Open(&SCREEN_SPI_CTRL, &SCREEN_SPI_CFG);
    if (err != FSP_SUCCESS) {
        return (int)err;
    }
    if (s_spi_done_sem == NULL) {
        s_spi_done_sem = xSemaphoreCreateBinaryStatic(&s_spi_done_sem_buf);
        if (s_spi_done_sem == NULL) {
            R_SPI_Close(&SCREEN_SPI_CTRL);
            return (int)FSP_ERR_OUT_OF_MEMORY;
        }
    }
    return 0;
}
