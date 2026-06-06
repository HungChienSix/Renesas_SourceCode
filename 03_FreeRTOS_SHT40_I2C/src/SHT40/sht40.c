/* V1.0 SHT40 Temperature & Humidity Sensor Driver */

#include "sht40.h"
#include <stddef.h>
#include <stdio.h>

/* I2C 单次传输阻塞超时: 100 ms 远大于典型 1 ms 测量/读取 */
#define SHT40_I2C_TIMEOUT_US  100000U
#include <math.h>      /* logf for dew point */

/* ========== 内部状态 (与 BMP280 / MAX30102 风格统一) ========== */
static bool    g_initialized = false;
static uint32_t g_serial     = 0;   /* 调试用,记录芯片序列号 */

/* 通过平台接口获取硬件操作函数 (返回通用 i2c_port) */
static inline const i2c_port_t * SHT40_I2cGet(void) {
    return i2c_port_get();
}

/* ========== CRC8 (多项式 0x31, 初值 0xFF, 来自数据手册) ========== */
uint8_t SHT40_CRC8(const uint8_t *data, size_t len) {
    uint8_t crc = 0xFF;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (uint8_t b = 0; b < 8; b++) {
            if (crc & 0x80) {
                crc = (uint8_t)((crc << 1) ^ 0x31);
            } else {
                crc = (uint8_t)(crc << 1);
            }
        }
    }
    return crc;
}

/* ========== 物理量转换 (来自数据手册) ========== */

float SHT40_ConvertTemperature(uint16_t raw_t) {
    return -45.0f + 175.0f * ((float)raw_t / 65535.0f);
}

float SHT40_ConvertHumidity(uint16_t raw_rh) {
    float rh = -6.0f + 125.0f * ((float)raw_rh / 65535.0f);
    if (rh < 0.0f)   rh = 0.0f;
    if (rh > 100.0f) rh = 100.0f;
    return rh;
}

/* 露点温度(Magnus 公式,精度 ±0.4°C)
 *  α = (a·T)/(b+T) + ln(RH/100)
 *  Td = (b·α)/(a - α)
 *  a = 17.27, b = 237.7°C (适用于 T 在 [-40, 50] °C) */
float SHT40_ConvertDewPoint(float temperature_c, float humidity_pct) {
    if (humidity_pct <= 0.0f)   return -273.15f;   /* 无效: 干空气 */
    if (humidity_pct >= 100.0f) return temperature_c;

    const float a = 17.27f;
    const float b = 237.7f;
    float alpha = (a * temperature_c) / (b + temperature_c)
                + logf(humidity_pct / 100.0f);
    return (b * alpha) / (a - alpha);
}

/* ========== 内部: 写命令 (发 1 字节, SHT4x 接受 LSB 命令码) ========== */
static SHT40_Status_t SHT40_WriteCommand(uint8_t cmd) {
    const i2c_port_t *p = SHT40_I2cGet();
    int err = p->write(SHT40_I2C_ADDR, &cmd, 1, false, SHT40_I2C_TIMEOUT_US);  // restart=false: 末尾发 STOP
    return (err == 0) ? SHT40_OK : SHT40_ERR_I2C;
}

/* ========== 内部: 根据精度取测量时间 ========== */
static uint32_t SHT40_MeasurementDelayMs(SHT40_Precision_t precision) {
    switch (precision) {
        case SHT40_PRECISION_HIGH:   return SHT40_MEAS_TIME_HIGH_PRECISION;
        case SHT40_PRECISION_MEDIUM: return SHT40_MEAS_TIME_MEDIUM_PRECISION;
        case SHT40_PRECISION_LOW:    return SHT40_MEAS_TIME_LOW_PRECISION;
        default:                     return SHT40_MEAS_TIME_HIGH_PRECISION;
    }
}

/* ========== 对外 API ========== */

fsp_err_t SHT40_Init(void) {
    fsp_err_t err = (fsp_err_t)i2c_port_init();  // 打开 I2C 外设
    if (err != FSP_SUCCESS) return err;

    /* 软复位, 即便失败也不阻塞主循环 (例如传感器未接) */
    SHT40_SoftReset();

    /* 顺便读一次序列号存到 g_serial,DebugDump 时打印 */
    (void)SHT40_ReadSerial(&g_serial);

    g_initialized = true;
    return FSP_SUCCESS;
}

SHT40_Status_t SHT40_SoftReset(void) {
    SHT40_Status_t ret = SHT40_WriteCommand(SHT40_CMD_SOFT_RESET);
    if (ret != SHT40_OK) return ret;
    SHT40_I2cGet()->delay_ms(SHT40_SOFT_RESET_TIME_MS);
    return SHT40_OK;
}

SHT40_Status_t SHT40_ReadSerial(uint32_t *serial) {
    if (serial == NULL) return SHT40_ERR_PARAM;
    const i2c_port_t *p = SHT40_I2cGet();

    SHT40_Status_t ret = SHT40_WriteCommand(SHT40_CMD_READ_SERIAL);
    if (ret != SHT40_OK) return ret;

    uint8_t buf[SHT40_READ_BUFFER_SIZE] = {0};
    int err = p->read(SHT40_I2C_ADDR, buf, sizeof(buf), false, SHT40_I2C_TIMEOUT_US);  // restart=false: 末尾发 STOP
    if (err != 0) return SHT40_ERR_I2C;

    /* 校验两个 CRC */
    if (SHT40_CRC8(&buf[0], 2) != buf[2]) return SHT40_ERR_CRC;
    if (SHT40_CRC8(&buf[3], 2) != buf[5]) return SHT40_ERR_CRC;

    *serial = ((uint32_t)buf[0] << 24) | ((uint32_t)buf[1] << 16) |
              ((uint32_t)buf[3] <<  8) | ((uint32_t)buf[4] <<  0);
    return SHT40_OK;
}

SHT40_Status_t SHT40_ReadMeasurement(SHT40_Precision_t precision, SHT40_Data_t *data) {
    if (data == NULL) return SHT40_ERR_PARAM;
    const i2c_port_t *p = SHT40_I2cGet();

    /* 1. 发送测量命令 (1 字节 LSB=命令码, restart=true 保持总线) */
    uint8_t cmd = (uint8_t)precision;
    int err = p->write(SHT40_I2C_ADDR, &cmd, 1, true, SHT40_I2C_TIMEOUT_US);
    if (err != 0) return SHT40_ERR_I2C;

    /* 2. 等待测量完成 (数据手册典型值 + 余量) */
    p->delay_ms(SHT40_MeasurementDelayMs(precision));

    /* 3. 读回 6 字节: T(MSB,LSB,CRC) + RH(MSB,LSB,CRC) */
    uint8_t buf[SHT40_READ_BUFFER_SIZE] = {0};
    err = p->read(SHT40_I2C_ADDR, buf, sizeof(buf), false, SHT40_I2C_TIMEOUT_US);  // restart=false: 末尾发 STOP
    if (err != 0) return SHT40_ERR_I2C;

    /* 4. 校验 CRC */
    if (SHT40_CRC8(&buf[0], 2) != buf[2]) return SHT40_ERR_CRC;
    if (SHT40_CRC8(&buf[3], 2) != buf[5]) return SHT40_ERR_CRC;

    /* 5. 拼装并转换 */
    uint16_t raw_t  = (uint16_t)((buf[0] << 8) | buf[1]);
    uint16_t raw_rh = (uint16_t)((buf[3] << 8) | buf[4]);
    data->temperature_c = SHT40_ConvertTemperature(raw_t);
    data->humidity_pct  = SHT40_ConvertHumidity(raw_rh);
    return SHT40_OK;
}

SHT40_Status_t SHT40_ReadHighPrecision(SHT40_Data_t *data) {
    return SHT40_ReadMeasurement(SHT40_PRECISION_HIGH, data);
}

SHT40_Status_t SHT40_ReadMediumPrecision(SHT40_Data_t *data) {
    return SHT40_ReadMeasurement(SHT40_PRECISION_MEDIUM, data);
}

SHT40_Status_t SHT40_ReadLowPrecision(SHT40_Data_t *data) {
    return SHT40_ReadMeasurement(SHT40_PRECISION_LOW, data);
}

/* ========== 诊断: 单步打印每一步的错误, 定位 read error 卡在哪 ========== */
void SHT40_DebugReadStep(void) {
    const i2c_port_t *p = SHT40_I2cGet();

    /* 步骤 1: 软复位 */
    SHT40_Status_t r1 = SHT40_SoftReset();
    printf("  [1] SoftReset: %d (0=OK)\n", (int)r1);
    if (r1 != SHT40_OK) return;

    /* 步骤 2: 写测量命令 (1 字节 0xFD, restart=true 保持总线) */
    uint8_t cmd = SHT40_CMD_MEAS_HIGH_PRECISION;
    int e2 = p->write(SHT40_I2C_ADDR, &cmd, 1, true, SHT40_I2C_TIMEOUT_US);
    printf("  [2] Write cmd [0x%02X]: %d (0=OK, 18=ABORTED=NACK, 20=TIMEOUT)\n", cmd, e2);
    if (e2 != 0) return;

    /* 步骤 3: 等待测量 */
    p->delay_ms(15);
    printf("  [3] Wait 15ms done\n");

    /* 步骤 4: 读 6 字节 (restart=false) */
    uint8_t buf[6] = {0};
    int e4 = p->read(SHT40_I2C_ADDR, buf, 6, false, SHT40_I2C_TIMEOUT_US);
    printf("  [4] Read 6B: %d, bytes=", e4);
    for (int i = 0; i < 6; i++) printf("%02X ", buf[i]);
    printf("\n");
    if (e4 != 0) return;

    /* 步骤 5: CRC 校验 */
    uint8_t crc_t = SHT40_CRC8(&buf[0], 2);
    uint8_t crc_rh = SHT40_CRC8(&buf[3], 2);
    printf("  [5] CRC: T=%02X(exp %02X) RH=%02X(exp %02X)\n",
           crc_t, buf[2], crc_rh, buf[5]);
    if (crc_t != buf[2] || crc_rh != buf[5]) {
        printf("  !! CRC mismatch, raw data is corrupt\n");
        return;
    }

    /* 步骤 6: 转换 (printf 不支持 %f, 用整数 *100 表示, 即 2565 = 25.65) */
    uint16_t raw_t  = (buf[0] << 8) | buf[1];
    uint16_t raw_rh = (buf[3] << 8) | buf[4];
    int t_x100  = -4500 + (int)((17500LL * (int)raw_t) / 65535);
    int rh_x100 = -600  + (int)((12500LL * (int)raw_rh) / 65535);
    if (rh_x100 < 0)     rh_x100 = 0;
    if (rh_x100 > 10000) rh_x100 = 10000;
    printf("  [6] Raw T=%u RH=%u -> T=%d.%02d C RH=%d.%02d %%\n",
           raw_t, raw_rh,
           t_x100 / 100,  t_x100  % 100,
           rh_x100 / 100, rh_x100 % 100);
}

/* ========== 诊断: 一次性打印 (与 BMP280_DebugDump / MAX30102_DebugDump 风格统一) ========== */
void SHT40_DebugDump(void) {
    printf("[SHT40] serial: 0x%08X  init: %s\n",
           g_serial, g_initialized ? "OK" : "FAIL");
    SHT40_DebugReadStep();
}
