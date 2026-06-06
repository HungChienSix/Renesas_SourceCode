/* V1.0 Screen Porting Layer - 通用屏幕 SPI/GPIO 抽象,所有屏幕驱动共用 */
#ifndef SCREEN_PORT_H
#define SCREEN_PORT_H

#include "hal_data.h"
#include <stdint.h>
#include <stdbool.h>

#define SCREEN_SPI_CTRL     g_spi0_ctrl
#define SCREEN_SPI_CFG      g_spi0_cfg

#define SCREEN_RES_Pin      (BSP_IO_PORT_00_PIN_01)  // P001
#define SCREEN_DC_Pin       (BSP_IO_PORT_04_PIN_14)  // P414
#define SCREEN_CS_Pin       (BSP_IO_PORT_04_PIN_05)  // P405
#define SCREEN_BLK_Pin      (BSP_IO_PORT_04_PIN_15)  // P211
#define SCREEN_RXD_Pin      (BSP_IO_PORT_02_PIN_02)  // P202 MISO
#define SCREEN_SCK_Pin      (BSP_IO_PORT_02_PIN_04)  // P204
#define SCREEN_TXD_Pin      (BSP_IO_PORT_02_PIN_03)  // P203 MOSI

/**
 * @brief SPI 发送函数类型 - 平台实现 (R_SPI 接口语义)
 * @param p_src       待发送数据缓冲区
 * @param length      数据长度 (字节)
 * @param timeout_us  阻塞等待超时 (微秒), 0 表示非阻塞, UINT32_MAX 表示永久等待
 * @return 0 成功, 非 0 失败
 */
typedef int (*screen_spi_transfer_t)(void const * p_src, uint32_t length, uint32_t timeout_us);

/**
 * @brief GPIO 写函数类型 - 平台实现
 * @param pin   BSP 引脚句柄
 * @param level 电平 0/1
 */
typedef void (*screen_gpio_write_t)(uint32_t pin, uint8_t level);

/**
 * @brief 毫秒延时
 */
typedef void (*screen_delay_ms_t)(uint32_t ms);

/**
 * @brief 微秒延时
 */
typedef void (*screen_delay_us_t)(uint32_t us);

/**
 * @brief 屏幕硬件平台接口
 */
typedef struct screen_port {
    screen_spi_transfer_t spi_transfer;   // SPI 发送 (阻塞/超时)
    screen_gpio_write_t   gpio_write;     // GPIO 写电平
    screen_delay_ms_t     delay_ms;       // 毫秒延时
    screen_delay_us_t     delay_us;       // 微秒延时
} screen_port_t;

/**
 * @brief 获取当前屏幕平台接口
 */
const screen_port_t * screen_port_get(void);

/**
 * @brief 初始化屏幕硬件平台 (打开 SPI 外设 + 创建同步原语)
 * @return 0 成功, 非 0 失败
 */
int screen_port_init(void);

#endif /* SCREEN_PORT_H */
