/* V1.0 MPU6050 Porting Layer - Renesas RA Implementation */

#include "mpu6050_port.h"

/* I2C传输完成标志 */
volatile bool g_mpu6050_port_i2c_receive_complete_flag = false;
volatile bool g_mpu6050_port_i2c_send_complete_flag = false;

/* I2C主模式回调函数 */
void sci_i2c_master_callback(i2c_master_callback_args_t *p_args)
{
    switch (p_args->event)
    {
        case I2C_MASTER_EVENT_RX_COMPLETE:
        {
            g_mpu6050_port_i2c_receive_complete_flag = true;
            break;
        }
        case I2C_MASTER_EVENT_TX_COMPLETE:
        {
            g_mpu6050_port_i2c_send_complete_flag = true;
            break;
        }
        default:
            break;
    }
}

/* I2C写函数实现 */
static uint8_t MPU6050_PORT_I2cWrite(uint8_t *buf, uint8_t len, bool restart)
{
    g_mpu6050_port_i2c_send_complete_flag = false;
    fsp_err_t err = R_SCI_I2C_Write(&MPU_I2C_CTRL, buf, len, restart);
    if (err != FSP_SUCCESS)
        return (uint8_t)err;
    while (!g_mpu6050_port_i2c_send_complete_flag)
        ;
    return 0;
}

/* I2C读函数实现 */
static uint8_t MPU6050_PORT_I2cRead(uint8_t *buf, uint8_t len, bool restart)
{
    g_mpu6050_port_i2c_receive_complete_flag = false;
    fsp_err_t err = R_SCI_I2C_Read(&MPU_I2C_CTRL, buf, len, restart);
    if (err != FSP_SUCCESS)
        return (uint8_t)err;
    while (!g_mpu6050_port_i2c_receive_complete_flag)
        ;
    return 0;
}

/* MPU6050平台接口 - Renesas RA */
static const mpu6050_port_t g_mpu6050_port = {
    .i2c_write = MPU6050_PORT_I2cWrite,
    .i2c_read  = MPU6050_PORT_I2cRead,
};

const mpu6050_port_t * mpu6050_port_get(void)
{
    return &g_mpu6050_port;
}

int mpu6050_port_init(void)
{
    fsp_err_t err = R_SCI_I2C_Open(&MPU_I2C_CTRL, &MPU_I2C_CFG);
    return (err == FSP_SUCCESS) ? 0 : (int)err;
}
