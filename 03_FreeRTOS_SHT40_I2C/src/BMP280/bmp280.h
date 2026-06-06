/* V1.0 BMP280 Pressure & Temperature Sensor Driver
 *
 * Bosch BMP280:高精度大气压力 + 温度传感器
 *  - 压力精度 ±1 hPa(用 24-bit ADC,过采样后可达 ±0.12 hPa ≈ ±1 m 海拔)
 *  - 温度精度 ±0.01 °C(用于气压温度补偿,本驱动也输出)
 *  - 7-bit I2C 地址 0x76 (SDO=GND) 或 0x77 (SDO=VDD)
 *  - 数据手册 Rev 1.6 (BST-BMP280-DS002)
 *
 * 本驱动采用 Bosch 官方 32-bit 整数补偿算法,纯整数无浮点。
 * I2C 访问走通用 hal_port/i2c_port,延时也由 i2c_port 提供,
 * 因此本文件只包含器件协议与补偿逻辑,不依赖任何平台符号。
 */

#ifndef BMP280_H
#define BMP280_H

#include "hal_data.h"
#include "hal_port/i2c_port.h"   /* 通用 I2C 抽象,所有传感器共用 */
#include <stdint.h>
#include <stdbool.h>

/* ========== 器件地址与 ID ========== */
#define BMP280_I2C_ADDR_PRIMARY   0x76    // SDO 接 GND
#define BMP280_I2C_ADDR_SECONDARY 0x77    // SDO 接 VDD
#define BMP280_I2C_ADDR           BMP280_I2C_ADDR_PRIMARY   /* 本工程默认地址 */
#define BMP280_CHIP_ID            0x58    /* reg 0xD0 应返回此值 */

/* ========== 寄存器地址 ========== */
#define BMP280_REG_ID            0xD0     /* 芯片 ID */
#define BMP280_REG_RESET         0xE0     /* 软复位,写入 0xB6 */
#define BMP280_REG_STATUS        0xF3     /* 状态:bit0=im_update, bit3=measuring */
#define BMP280_REG_CTRL_MEAS     0xF4     /* osrs_t[7:5] osrs_p[4:2] mode[1:0] */
#define BMP280_REG_CONFIG        0xF5     /* t_sb[7:5] filter[4:2] spi3w[0] */
#define BMP280_REG_PRESS_MSB     0xF7     /* 压力 3 字节 (MSB/LSB/xLSB) */
#define BMP280_REG_TEMP_MSB      0xFA     /* 温度 3 字节 (MSB/LSB/xLSB) */

/* 软复位命令字 */
#define BMP280_SOFT_RESET_CMD    0xB6

/* 校准数据起始地址 + 长度 (26 字节, 0x88~0xA1) */
#define BMP280_REG_CALIB_START   0x88
#define BMP280_REG_CALIB_LEN     26

/* ========== CTRL_MEAS 配置位掩码 ========== */
#define BMP280_OSRS_T_SKIP       (0 << 5) /* 温度跳过测量 */
#define BMP280_OSRS_T_x1         (1 << 5)
#define BMP280_OSRS_T_x2         (2 << 5)
#define BMP280_OSRS_T_x4         (3 << 5)
#define BMP280_OSRS_T_x8         (4 << 5)
#define BMP280_OSRS_T_x16        (5 << 5)

#define BMP280_OSRS_P_SKIP       (0 << 2) /* 压力跳过测量 */
#define BMP280_OSRS_P_x1         (1 << 2)
#define BMP280_OSRS_P_x2         (2 << 2)
#define BMP280_OSRS_P_x4         (3 << 2)
#define BMP280_OSRS_P_x8         (4 << 2)
#define BMP280_OSRS_P_x16        (5 << 2)

#define BMP280_MODE_SLEEP        (0 << 0)
#define BMP280_MODE_FORCED       (1 << 0)
#define BMP280_MODE_NORMAL       (3 << 0)

/* ========== CONFIG 寄存器位掩码 ========== */
/* t_sb[7:5] standby 时间(仅 normal 模式有效) */
#define BMP280_STANDBY_0_5_MS    (0 << 5)
#define BMP280_STANDBY_62_5_MS   (1 << 5)
#define BMP280_STANDBY_125_MS    (2 << 5)
#define BMP280_STANDBY_250_MS    (3 << 5)
#define BMP280_STANDBY_500_MS    (4 << 5)
#define BMP280_STANDBY_1000_MS   (5 << 5)
#define BMP280_STANDBY_2000_MS   (6 << 5)
#define BMP280_STANDBY_4000_MS   (7 << 5)

/* filter[4:2] IIR 滤波器系数 */
#define BMP280_FILTER_OFF        (0 << 2)
#define BMP280_FILTER_COEFF_2    (1 << 2)
#define BMP280_FILTER_COEFF_4    (2 << 2)
#define BMP280_FILTER_COEFF_8    (3 << 2)
#define BMP280_FILTER_COEFF_16   (4 << 2)

/* ========== 测量延迟(经验值,含启动 + 转换 + 缓冲) ========== */
/* 启动时间典型 2 ms,osrs_t=x2 转换时间约 1.3 ms,osrs_p=x16 约 10.4 ms
 * 取保守 15 ms,实测无问题 */
#define BMP280_MEAS_TIME_MS      15
#define BMP280_SOFT_RESET_TIME_MS 2

/* ========== 驱动状态码 ========== */
typedef enum {
    BMP280_OK         =  0,    /* 成功 */
    BMP280_ERR_I2C    = -1,    /* I2C 通信失败 */
    BMP280_ERR_ID     = -2,    /* 读到的 ID 不是 0x58 */
    BMP280_ERR_PARAM  = -3,    /* 参数错误 */
} BMP280_Status_t;

/* ========== 校准系数(从 reg 0x88 读出的原始值) ========== */
typedef struct {
    uint16_t dig_T1;   /* 0x88 / 0x89 */
    int16_t  dig_T2;   /* 0x8A / 0x8B */
    int16_t  dig_T3;   /* 0x8C / 0x8D */
    uint16_t dig_P1;   /* 0x8E / 0x8F */
    int16_t  dig_P2;   /* 0x90 / 0x91 */
    int16_t  dig_P3;   /* 0x92 / 0x93 */
    int16_t  dig_P4;   /* 0x94 / 0x95 */
    int16_t  dig_P5;   /* 0x96 / 0x97 */
    int16_t  dig_P6;   /* 0x98 / 0x99 */
    int16_t  dig_P7;   /* 0x9A / 0x9B */
    int16_t  dig_P8;   /* 0x9C / 0x9D */
    int16_t  dig_P9;   /* 0x9E / 0x9F */
} BMP280_Calib_t;

/* ========== 测量结果 ========== */
typedef struct {
    int32_t  temperature_centi_c;   /* 温度,0.01 °C 单位 (例: 2567 = 25.67 °C) */
    uint32_t pressure_pa;           /* 压力,Pa 单位 (例: 101325 = 1 标准大气压) */
    /* 由压力 + 当前海拔参考反算的海拔(单位 0.01 m,例: 1234 = 12.34 m) */
    int32_t  altitude_cm;
} BMP280_Data_t;

/* ========== 对外 API ========== */

/**
 * @brief 一次性完成 BMP280 初始化:读 ID → 读校准 → 软复位 → 配置
 * @return FSP_SUCCESS 或 FSP_ERR_*
 */
fsp_err_t BMP280_Init(void);

/**
 * @brief 软复位 (写 0xB6 到 0xE0)
 */
BMP280_Status_t BMP280_SoftReset(void);

/**
 * @brief 触发一次 forced 测量,完成后读回 T + P 并补偿
 * @param data  输出
 */
BMP280_Status_t BMP280_ReadForced(BMP280_Data_t *data);

/**
 * @brief 读芯片 ID (调试用)
 * @param id  输出 0x58
 */
BMP280_Status_t BMP280_ReadID(uint8_t *id);

/**
 * @brief 读状态寄存器 (bit3=measuring, bit0=im_update)
 * @param status  输出
 */
BMP280_Status_t BMP280_ReadStatus(uint8_t *status);

/**
 * @brief 用当前 sea_level_pa (海平面气压) 计算海拔(cm)
 *        标准海平面 101325 Pa;下雨/晴天差异 ±200 Pa
 *        用国际气压公式,误差 < 0.1 m (海平面附近)
 */
int32_t BMP280_CalcAltitudeCm(uint32_t pressure_pa, uint32_t sea_level_pa);

/**
 * @brief 用更简单的线性公式估算海拔(cm)
 *        公式: alt = (sea_level - pressure) * 0.0843 m/Pa * 100
 *        误差 ±1 m,适合快速计算(无 pow)
 */
int32_t BMP280_CalcAltitudeCmFast(uint32_t pressure_pa, uint32_t sea_level_pa);

/**
 * @brief 全局海平面基准(默认值 101325 Pa)
 *        校准后调 BMP280_SetSeaLevelPa() 覆盖,后续 BMP280_ReadForced 自动用
 */
void       BMP280_SetSeaLevelPa(uint32_t sea_level_pa);
uint32_t   BMP280_GetSeaLevelPa(void);

/* ========== 补偿算法(公开,便于单元测试) ========== */

/**
 * @brief 温度补偿:输入 raw adc,输出 0.01°C 和 t_fine
 */
int32_t BMP280_CompensateTemperature(int32_t adc_T, int32_t *t_fine);

/**
 * @brief 压力补偿:输入 raw adc + t_fine,输出 Pa
 */
uint32_t BMP280_CompensatePressure(int32_t adc_P, int32_t t_fine);

#endif /* BMP280_H */
