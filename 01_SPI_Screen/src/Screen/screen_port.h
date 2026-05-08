/* V1.0 Screen Porting Layer */
#ifndef SCREEN_PORT_H
#define SCREEN_PORT_H

#include "hal_data.h"
#include <stdint.h>
#include <stdbool.h> 

#define SCREEN_SPI_CTRL       g_spi0_ctrl
#define SCREEN_SPI_CFG        g_spi0_cfg

#define TFT_RES_Pin    (BSP_IO_PORT_03_PIN_11)  // P311
#define TFT_DC_Pin     (BSP_IO_PORT_03_PIN_12)  // P312
#define TFT_CS_Pin     (BSP_IO_PORT_09_PIN_07)  // P907

/**
 * @brief SPI传输函数类型 - 平台实现
 * @param p_src 数据源缓冲区
 * @param length 传输长度
 * @param timeout_us 超时时间（微秒），0表示无限等待
 * @return true成功，false失败
 */
typedef bool (*screen_spi_transfer_t)(void const * p_src, uint32_t length, uint32_t timeout_us);

/**
 * @brief GPIO写函数类型 - 平台实现
 * @param pin _pin句柄
 * @param level 电平
 */
typedef void (*screen_gpio_write_t)(uint32_t pin, uint8_t level);

/**
 * @brief 延时函数类型 - 平台实现
 * @param delay 延时数值
 * @param units 延时单位
 */
typedef void (*screen_delay_t)(uint32_t delay, uint8_t units);

/**
 * @brief SPI传输完成回调函数类型
 * @param p_args 回调参数
 */
typedef void (*screen_spi_callback_t)(void * p_args);

/**
 * @brief 屏幕硬件平台接口
 */
typedef struct screen_port {
    screen_spi_transfer_t  spi_transfer;    // SPI传输函数
    screen_gpio_write_t   gpio_write;      // GPIO写函数
    screen_delay_t        delay_ms;        // 毫秒延时
    screen_delay_t        delay_us;        // 微秒延时
} screen_port_t;

/**
 * @brief 获取当前屏幕端口接口
 * @return 指向平台接口的指针
 */
const screen_port_t * screen_port_get(void);

/**
 * @brief 初始化屏幕硬件平台
 * @return 0成功，非0失败
 */
int screen_port_init(void);

#endif /* SCREEN_PORT_H */
