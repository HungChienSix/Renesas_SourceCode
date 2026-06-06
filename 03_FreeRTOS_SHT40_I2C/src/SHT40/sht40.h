/* V1.0 SHT40 Temperature & Humidity Sensor Driver */

#ifndef SHT40_H
#define SHT40_H

#include "hal_data.h"
#include "hal_port/i2c_port.h"   /* 通用 I2C 抽象,所有传感器共用 */
#include <stdint.h>
#include <stdbool.h>

/* SHT4x 测量命令 (LSB = 命令码; SHT4x 在 1 字节模式下也接受)
 * 数据手册写 2 字节 [MSB=cmd, LSB=0x00], 实测 SCI I2C + SHT4x 用 1 字节 (LSB) 也 OK */
#define SHT40_CMD_MEAS_HIGH_PRECISION   0xFD    // T+RH 高精度, 典型 6.9 ms
#define SHT40_CMD_MEAS_MEDIUM_PRECISION 0xF6    // T+RH 中精度, 典型 3.7 ms
#define SHT40_CMD_MEAS_LOW_PRECISION    0xE0    // T+RH 低精度, 典型 1.3 ms
#define SHT40_CMD_READ_SERIAL           0x89    // 读序列号
#define SHT40_CMD_SOFT_RESET            0x94    // 软复位, 等待时间典型 1 ms

/* SHT4x 7-bit I2C 地址 (默认 ADDR pin 接 GND = 0x44, 接 VDD = 0x45) */
#define SHT40_I2C_ADDR                  0x44

/* 测量完成时间 (ms), 来自数据手册,调用方需在读回前 delay */
#define SHT40_MEAS_TIME_HIGH_PRECISION  10
#define SHT40_MEAS_TIME_MEDIUM_PRECISION 5
#define SHT40_MEAS_TIME_LOW_PRECISION   2
#define SHT40_SOFT_RESET_TIME_MS        1

/* 数据格式: 6 字节 = [T_MSB, T_LSB, T_CRC, RH_MSB, RH_LSB, RH_CRC] */
#define SHT40_READ_BUFFER_SIZE          6

/* 驱动状态码 */
typedef enum {
    SHT40_OK          =  0,    // 成功
    SHT40_ERR_I2C     = -1,    // I2C 通信失败
    SHT40_ERR_CRC     = -2,    // CRC 校验失败
    SHT40_ERR_PARAM   = -3,    // 参数错误
} SHT40_Status_t;

/* 测量精度选择 (与命令一一对应) */
typedef enum {
    SHT40_PRECISION_HIGH   = SHT40_CMD_MEAS_HIGH_PRECISION,
    SHT40_PRECISION_MEDIUM = SHT40_CMD_MEAS_MEDIUM_PRECISION,
    SHT40_PRECISION_LOW    = SHT40_CMD_MEAS_LOW_PRECISION,
} SHT40_Precision_t;

/* 温湿度结果 (单位: 摄氏度 / %RH) */
typedef struct {
    float temperature_c;
    float humidity_pct;
} SHT40_Data_t;

/* ========== 初始化 ========== */

/**
 * @brief 一次性完成 SHT40 初始化 (打开 I2C, 软复位)
 * @return FSP_SUCCESS 或 FSP_ERR_*
 */
fsp_err_t SHT40_Init(void);

/* ========== 测量 API ========== */

/**
 * @brief 软复位 SHT40
 */
SHT40_Status_t SHT40_SoftReset(void);

/**
 * @brief 读取 32-bit 序列号
 */
SHT40_Status_t SHT40_ReadSerial(uint32_t *serial);

/**
 * @brief 单次测量 (指定精度), 内部自动等待测量完成
 */
SHT40_Status_t SHT40_ReadMeasurement(SHT40_Precision_t precision, SHT40_Data_t *data);

/**
 * @brief 高精度单次测量快捷接口
 */
SHT40_Status_t SHT40_ReadHighPrecision(SHT40_Data_t *data);

/**
 * @brief 中精度单次测量快捷接口
 */
SHT40_Status_t SHT40_ReadMediumPrecision(SHT40_Data_t *data);

/**
 * @brief 低精度单次测量快捷接口
 */
SHT40_Status_t SHT40_ReadLowPrecision(SHT40_Data_t *data);

/* ========== 工具函数 ========== */

/**
 * @brief SHT4x CRC8 计算 (多项式 0x31, 初始值 0xFF)
 */
uint8_t SHT40_CRC8(const uint8_t *data, size_t len);

/**
 * @brief 原始温度值转摄氏度
 * @param raw_t 16-bit 原始温度值
 */
float SHT40_ConvertTemperature(uint16_t raw_t);

/**
 * @brief 原始湿度值转 %RH (结果已 clamp 到 [0, 100])
 * @param raw_rh 16-bit 原始湿度值
 */
float SHT40_ConvertHumidity(uint16_t raw_rh);

/**
 * @brief 露点温度(°C) Magnus 公式
 * @param temperature_c  当前温度 (°C)
 * @param humidity_pct   当前相对湿度 (%)
 * @return 露点温度 (°C),误差 ±0.4°C, 范围 [-40, 50]
 *
 *  公式: α = (a·T)/(b+T) + ln(RH/100)
 *        Td = (b·α)/(a-α)
 *  其中 a=17.27, b=237.7°C
 */
float SHT40_ConvertDewPoint(float temperature_c, float humidity_pct);

/* 诊断: 单步打印软复位/写命令/读/CRC/转换的每一步状态 */
void SHT40_DebugReadStep(void);

#endif /* SHT40_H */
