/* V1.0 I2C Porting Layer - 通用 I2C 抽象,所有传感器共用 */
#ifndef I2C_PORT_H
#define I2C_PORT_H

#include "hal_data.h"
#include <stdint.h>
#include <stdbool.h>

/**
 * @brief I2C 写入函数类型 - 平台实现 (R_SCI_I2C 接口语义)
 * @param addr        从设备 7-bit 地址
 * @param data        待发送数据
 * @param len         数据长度
 * @param restart     true=保持总线不发 STOP (用于后续读), false=本帧结束发 STOP
 * @param timeout_us  阻塞等待超时 (微秒), 0=非阻塞, UINT32_MAX=永久
 * @return 0 成功, 非 0 失败
 */
typedef int (*i2c_write_t)(uint8_t addr, const uint8_t *data, uint32_t len, bool restart, uint32_t timeout_us);

/**
 * @brief I2C 读取函数类型 - 平台实现 (R_SCI_I2C 接口语义)
 * @param addr        从设备 7-bit 地址
 * @param data        接收缓冲区
 * @param len         数据长度
 * @param restart     true=保持总线不发 STOP, false=本帧结束发 STOP
 * @param timeout_us  阻塞等待超时 (微秒), 0=非阻塞, UINT32_MAX=永久
 * @return 0 成功, 非 0 失败
 */
typedef int (*i2c_read_t)(uint8_t addr, uint8_t *data, uint32_t len, bool restart, uint32_t timeout_us);

/**
 * @brief 毫秒延时
 */
typedef void (*i2c_delay_ms_t)(uint32_t ms);

/**
 * @brief 微秒延时
 */
typedef void (*i2c_delay_us_t)(uint32_t us);

/**
 * @brief I2C 硬件平台接口
 */
typedef struct i2c_port {
    i2c_write_t      write;     // I2C 写入
    i2c_read_t       read;      // I2C 读取
    i2c_delay_ms_t   delay_ms;  // 毫秒延时
    i2c_delay_us_t   delay_us;  // 微秒延时
} i2c_port_t;

/**
 * @brief 获取当前 I2C 平台接口
 */
const i2c_port_t * i2c_port_get(void);

/**
 * @brief 初始化 I2C 硬件平台 (打开外设)
 * @return 0 成功, 非 0 失败
 */
int i2c_port_init(void);

/**
 * @brief 诊断工具: 扫描 0x03~0x77 全部 I2C 地址, 打印 ACK 的从机
 * @return 发现的设备数量
 */
int i2c_port_scan(void);

#endif /* I2C_PORT_H */
