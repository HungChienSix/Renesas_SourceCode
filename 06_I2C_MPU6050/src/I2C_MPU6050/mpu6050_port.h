/* V1.0 MPU6050 Porting Layer */
#ifndef MPU6050_PORT_H
#define MPU6050_PORT_H

#include "hal_data.h"
#include <stdint.h>
#include <stdbool.h>

/* I2C硬件配置 - 从hal_data获取 */
#define MPU_I2C_CTRL       g_i2c0_ctrl
#define MPU_I2C_CFG        g_i2c0_cfg

/**
 * @brief I2C写函数类型 - 平台实现
 * @param buf 数据缓冲区
 * @param len 数据长度
 * @param restart 是否发送restart条件
 * @return FSP错误码，0成功
 */
typedef uint8_t (*mpu6050_i2c_write_t)(uint8_t *buf, uint8_t len, bool restart);

/**
 * @brief I2C读函数类型 - 平台实现
 * @param buf 数据缓冲区
 * @param len 数据长度
 * @param restart 是否发送restart条件
 * @return FSP错误码，0成功
 */
typedef uint8_t (*mpu6050_i2c_read_t)(uint8_t *buf, uint8_t len, bool restart);

/**
 * @brief MPU6050硬件平台接口
 */
typedef struct mpu6050_port {
    mpu6050_i2c_write_t  i2c_write;    // I2C写函数
    mpu6050_i2c_read_t   i2c_read;     // I2C读函数
} mpu6050_port_t;

/**
 * @brief 获取当前MPU6050端口接口
 * @return 指向平台接口的指针
 */
const mpu6050_port_t * mpu6050_port_get(void);

/**
 * @brief 初始化MPU6050硬件平台
 * @return 0成功，非0失败
 */
int mpu6050_port_init(void);

#endif /* MPU6050_PORT_H */
