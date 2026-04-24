#include "mpu6050.h"
#include <string.h>

/* I2C传输完成标志 */
static volatile bool g_i2c_completed = false;
static volatile i2c_master_event_t g_i2c_event;

/* I2C主模式回调函数 */
void sci_i2c_master_callback(i2c_master_callback_args_t *p_args)
{
    g_i2c_event = p_args->event;
    g_i2c_completed = true;
}

/* 阻塞式I2C写 */
static uint8_t mpu_i2c_write(uint8_t *buf, uint8_t len, bool restart)
{
    g_i2c_completed = false;
    fsp_err_t err = R_SCI_I2C_Write(&g_i2c0_ctrl, buf, len, restart);
    if (err != FSP_SUCCESS)
        return 1;
    while (!g_i2c_completed)
        ;
    return (g_i2c_event == I2C_MASTER_EVENT_TX_COMPLETE) ? 0 : 1;
}

/* 阻塞式I2C读 */
static uint8_t mpu_i2c_read(uint8_t *buf, uint8_t len, bool restart)
{
    g_i2c_completed = false;
    fsp_err_t err = R_SCI_I2C_Read(&g_i2c0_ctrl, buf, len, restart);
    if (err != FSP_SUCCESS)
        return 1;
    while (!g_i2c_completed)
        ;
    return (g_i2c_event == I2C_MASTER_EVENT_RX_COMPLETE) ? 0 : 1;
}

/**********************************************
函数名称：MPU_Init
函数功能：初始化MPU6050
函数参数：无
函数返回值：0,初始化成功  其他,初始化失败
**********************************************/
uint8_t MPU_Init(void)
{
    uint8_t res;

    fsp_err_t err = R_SCI_I2C_Open(&g_i2c0_ctrl, &g_i2c0_cfg);
    if (err != FSP_SUCCESS)
        return 1;

    MPU_Write_Byte(MPU_PWR_MGMT1_REG, 0X80);
    R_BSP_SoftwareDelay(100, BSP_DELAY_UNITS_MILLISECONDS);
    MPU_Write_Byte(MPU_PWR_MGMT1_REG, 0X00);
    MPU_Set_Gyro_Fsr(3);
    MPU_Set_Accel_Fsr(0);
    MPU_Set_Rate(50);
    MPU_Write_Byte(MPU_INT_EN_REG, 0X00);
    MPU_Write_Byte(MPU_USER_CTRL_REG, 0X00);
    MPU_Write_Byte(MPU_FIFO_EN_REG, 0X00);
    MPU_Write_Byte(MPU_INTBP_CFG_REG, 0X80);

    res = MPU_Read_Byte(MPU_DEVICE_ID_REG);
    if (res == MPU_ADDR)
    {
        MPU_Write_Byte(MPU_PWR_MGMT1_REG, 0X01);
        MPU_Write_Byte(MPU_PWR_MGMT2_REG, 0X00);
        MPU_Set_Rate(50);
    }
    else
        return 1;
    return 0;
}

/**********************************************
函数名称：MPU_Set_Gyro_Fsr
函数功能：设置陀螺仪满量程范围
函数参数：fsr:0,±250dps;1,±500dps;2,±1000dps;3,±2000dps
函数返回值：0,设置成功  其他,设置失败
**********************************************/
uint8_t MPU_Set_Gyro_Fsr(uint8_t fsr)
{
    return MPU_Write_Byte(MPU_GYRO_CFG_REG, fsr << 3);
}

/**********************************************
函数名称：MPU_Set_Accel_Fsr
函数功能：设置加速度传感器满量程范围
函数参数：fsr:0,±2g;1,±4g;2,±8g;3,±16g
函数返回值：0,设置成功  其他,设置失败
**********************************************/
uint8_t MPU_Set_Accel_Fsr(uint8_t fsr)
{
    return MPU_Write_Byte(MPU_ACCEL_CFG_REG, fsr << 3);
}

/**********************************************
函数名称：MPU_Set_LPF
函数功能：设置数字低通滤波器
函数参数：lpf:数字低通滤波频率(Hz)
函数返回值：0,设置成功  其他,设置失败
**********************************************/
uint8_t MPU_Set_LPF(uint16_t lpf)
{
    uint8_t data = 0;
    if (lpf >= 188) data = 1;
    else if (lpf >= 98) data = 2;
    else if (lpf >= 42) data = 3;
    else if (lpf >= 20) data = 4;
    else if (lpf >= 10) data = 5;
    else data = 6;
    return MPU_Write_Byte(MPU_CFG_REG, data);
}

/**********************************************
函数名称：MPU_Set_Rate
函数功能：设置采样率(假定Fs=1KHz)
函数参数：rate:4~1000(Hz)  初始化中rate取50
函数返回值：0,设置成功  其他,设置失败
**********************************************/
uint8_t MPU_Set_Rate(uint16_t rate)
{
    uint8_t data;
    if (rate > 1000) rate = 1000;
    if (rate < 4) rate = 4;
    data = 1000 / rate - 1;
    MPU_Write_Byte(MPU_SAMPLE_RATE_REG, data);
    return MPU_Set_LPF(rate / 2);
}

/**********************************************
函数名称：MPU_Get_Temperature
函数功能：得到温度传感器值
函数参数：无
函数返回值：温度值(扩大了100倍)
**********************************************/
short MPU_Get_Temperature(void)
{
    uint8_t buf[2];
    short raw;
    float temp;

    MPU_Read_Len(MPU_ADDR, MPU_TEMP_OUTH_REG, 2, buf);
    raw = ((uint16_t)buf[0] << 8) | buf[1];
    temp = 36.53 + ((double)raw) / 340;
    return temp * 100;
}

/**********************************************
函数名称：MPU_Get_Gyroscope
函数功能：得到陀螺仪值(原始值)
函数参数：gx,gy,gz:陀螺仪x,y,z轴的原始读数(带符号)
函数返回值：0,读取成功  其他,读取失败
**********************************************/
uint8_t MPU_Get_Gyroscope(short *gx, short *gy, short *gz)
{
    uint8_t buf[6], res;

    res = MPU_Read_Len(MPU_ADDR, MPU_GYRO_XOUTH_REG, 6, buf);
    if (res == 0)
    {
        *gx = ((uint16_t)buf[0] << 8) | buf[1];
        *gy = ((uint16_t)buf[2] << 8) | buf[3];
        *gz = ((uint16_t)buf[4] << 8) | buf[5];
    }
    return res;
}

/**********************************************
函数名称：MPU_Get_Accelerometer
函数功能：得到加速度值(原始值)
函数参数：ax,ay,az:加速度传感器x,y,z轴的原始读数(带符号)
函数返回值：0,读取成功  其他,读取失败
**********************************************/
uint8_t MPU_Get_Accelerometer(short *ax, short *ay, short *az)
{
    uint8_t buf[6], res;
    res = MPU_Read_Len(MPU_ADDR, MPU_ACCEL_XOUTH_REG, 6, buf);
    if (res == 0)
    {
        *ax = ((uint16_t)buf[0] << 8) | buf[1];
        *ay = ((uint16_t)buf[2] << 8) | buf[3];
        *az = ((uint16_t)buf[4] << 8) | buf[5];
    }
    return res;
}

/**********************************************
函数名称：MPU_Write_Len
函数功能：IIC连续写
函数参数：addr:器件地址(保留,已由FSP配置)
          reg:寄存器地址  len:数据长度  buf:数据区
函数返回值：0,写入成功  其他,写入失败
**********************************************/
uint8_t MPU_Write_Len(uint8_t addr, uint8_t reg, uint8_t len, uint8_t *buf)
{
    (void)addr;
    uint8_t write_buf[32];
    write_buf[0] = reg;
    memcpy(&write_buf[1], buf, len);
    return mpu_i2c_write(write_buf, len + 1, false);
}

/**********************************************
函数名称：MPU_Read_Len
函数功能：IIC连续读
函数参数：addr:器件地址(保留,已由FSP配置)
          reg:寄存器地址  len:数据长度  buf:数据存储区
函数返回值：0,读取成功  其他,读取失败
**********************************************/
uint8_t MPU_Read_Len(uint8_t addr, uint8_t reg, uint8_t len, uint8_t *buf)
{
    (void)addr;
    uint8_t res = mpu_i2c_write(&reg, 1, true);
    if (res != 0)
        return 1;
    return mpu_i2c_read(buf, len, false);
}

/**********************************************
函数名称：MPU_Write_Byte
函数功能：写一个字节到指定寄存器
函数参数：reg:寄存器地址  data:数据
函数返回值：0,写入成功  其他,写入失败
**********************************************/
uint8_t MPU_Write_Byte(uint8_t reg, uint8_t data)
{
    uint8_t buf[2] = {reg, data};
    return mpu_i2c_write(buf, 2, false);
}

/**********************************************
函数名称：MPU_Read_Byte
函数功能：从指定寄存器读一个字节
函数参数：reg:寄存器地址
函数返回值：读取到的数据
**********************************************/
uint8_t MPU_Read_Byte(uint8_t reg)
{
    uint8_t data;
    mpu_i2c_write(&reg, 1, true);
    mpu_i2c_read(&data, 1, false);
    return data;
}
