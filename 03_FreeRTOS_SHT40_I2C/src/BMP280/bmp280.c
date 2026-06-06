/* V1.0 BMP280 Driver - Bosch 官方 32-bit 整数补偿算法 */

#include "bmp280.h"
#include <stddef.h>
#include <stdio.h>
#include <math.h>      /* pow for full barometric formula */

/* I2C 单次传输阻塞超时: 100 ms 远大于典型 1 ms 测量/读取 */
#define BMP280_I2C_TIMEOUT_US  100000U

/* ========== 内部状态 ========== */
static BMP280_Calib_t g_calib;            /* 启动时读入,运行时只读 */
static uint8_t        g_chip_id = 0;      /* 用于诊断 */
static bool           g_initialized = false;

/* ========== 内部:取 I2C 平台接口 ========== */
static inline const i2c_port_t * BMP280_I2cGet(void) {
    return i2c_port_get();
}

/* ========== 内部:写 1 字节寄存器 ========== */
static BMP280_Status_t BMP280_WriteReg(uint8_t reg, uint8_t val) {
    const i2c_port_t *p = BMP280_I2cGet();
    uint8_t buf[2] = { reg, val };
    int err = p->write(BMP280_I2C_ADDR, buf, 2, false, BMP280_I2C_TIMEOUT_US);
    return (err == 0) ? BMP280_OK : BMP280_ERR_I2C;
}

/* ========== 内部:读 N 字节寄存器 (reg + data) ========== */
static BMP280_Status_t BMP280_ReadRegs(uint8_t reg, uint8_t *buf, uint32_t len) {
    const i2c_port_t *p = BMP280_I2cGet();

    /* 1. 写寄存器地址 (restart=true 保持总线, 接着读) */
    int err = p->write(BMP280_I2C_ADDR, &reg, 1, true, BMP280_I2C_TIMEOUT_US);
    if (err != 0) return BMP280_ERR_I2C;

    /* 2. 读 N 字节 (restart=false 末尾发 STOP) */
    err = p->read(BMP280_I2C_ADDR, buf, len, false, BMP280_I2C_TIMEOUT_US);
    return (err == 0) ? BMP280_OK : BMP280_ERR_I2C;
}

/* ========== 内部:读校准系数(26 字节, 0x88~0xA1) ========== */
static BMP280_Status_t BMP280_ReadCalibration(BMP280_Calib_t *cal) {
    uint8_t raw[BMP280_REG_CALIB_LEN] = {0};
    BMP280_Status_t r = BMP280_ReadRegs(BMP280_REG_CALIB_START, raw, sizeof(raw));
    if (r != BMP280_OK) return r;

    /* Bosch datasheet: little-endian 16-bit (LSB 在低地址) */
    cal->dig_T1 = (uint16_t)((raw[1]  << 8) | raw[0]);
    cal->dig_T2 = (int16_t) ((raw[3]  << 8) | raw[2]);
    cal->dig_T3 = (int16_t) ((raw[5]  << 8) | raw[4]);
    cal->dig_P1 = (uint16_t)((raw[7]  << 8) | raw[6]);
    cal->dig_P2 = (int16_t) ((raw[9]  << 8) | raw[8]);
    cal->dig_P3 = (int16_t) ((raw[11] << 8) | raw[10]);
    cal->dig_P4 = (int16_t) ((raw[13] << 8) | raw[12]);
    cal->dig_P5 = (int16_t) ((raw[15] << 8) | raw[14]);
    cal->dig_P6 = (int16_t) ((raw[17] << 8) | raw[16]);
    cal->dig_P7 = (int16_t) ((raw[19] << 8) | raw[18]);
    cal->dig_P8 = (int16_t) ((raw[21] << 8) | raw[20]);
    cal->dig_P9 = (int16_t) ((raw[23] << 8) | raw[22]);
    return BMP280_OK;
}

/* ========== 补偿算法 (来自 BMP280 datasheet Rev 1.6, §3.11.2) ========== */

int32_t BMP280_CompensateTemperature(int32_t adc_T, int32_t *t_fine) {
    int32_t var1, var2, T;

    var1 = ((((adc_T >> 3) - ((int32_t)g_calib.dig_T1 << 1))) *
            ((int32_t)g_calib.dig_T2)) >> 11;
    var2 = (((((adc_T >> 4) - ((int32_t)g_calib.dig_T1)) *
              ((adc_T >> 4) - ((int32_t)g_calib.dig_T1))) >> 12) *
            ((int32_t)g_calib.dig_T3)) >> 14;

    *t_fine = var1 + var2;
    T = (*t_fine * 5 + 128) >> 8;   /* 单位 0.01 °C, 例 2567 = 25.67 °C */
    return T;
}

uint32_t BMP280_CompensatePressure(int32_t adc_P, int32_t t_fine) {
    int64_t var1, var2, p;

    var1 = ((int64_t)t_fine) - 128000;
    var2 = var1 * var1 * (int64_t)g_calib.dig_P6;
    var2 = var2 + ((var1 * (int64_t)g_calib.dig_P5) << 17);
    var2 = var2 + (((int64_t)g_calib.dig_P4) << 35);
    var1 = ((var1 * var1 * (int64_t)g_calib.dig_P3) >> 8) +
           ((var1 * (int64_t)g_calib.dig_P2) << 12);
    var1 = (((((int64_t)1) << 47) + var1)) * ((int64_t)g_calib.dig_P1) >> 33;

    if (var1 == 0) return 0;   /* 避免除零 */

    p = 1048576 - adc_P;
    p = (((p << 31) - var2) * 3125) / var1;
    var1 = (((int64_t)g_calib.dig_P9) * (p >> 13) * (p >> 13)) >> 25;
    var2 = (((int64_t)g_calib.dig_P8) * p) >> 19;
    p = ((p + var1 + var2) >> 8) + (((int64_t)g_calib.dig_P7) << 4);

    return (uint32_t)(p / 256);   /* 单位 Pa */
}

/* ========== 海拔计算(国际气压公式) ========== */
/* 全局海平面基准 (供 BMP280_ReadForced 自动用) */
static uint32_t g_sea_level_pa = 101325;

void BMP280_SetSeaLevelPa(uint32_t sea_level_pa) {
    if (sea_level_pa > 0) g_sea_level_pa = sea_level_pa;
}
uint32_t BMP280_GetSeaLevelPa(void) {
    return g_sea_level_pa;
}

/* 简化版线性公式: 8.43 m / hPa,精度 ±1 m,无浮点运算 */
int32_t BMP280_CalcAltitudeCmFast(uint32_t pressure_pa, uint32_t sea_level_pa) {
    if (sea_level_pa == 0 || pressure_pa == 0) return 0;
    int32_t diff = (int32_t)sea_level_pa - (int32_t)pressure_pa;
    return (diff * 843) / 100;   /* ×8.43 ×100 → 0.01 m */
}

/* 完整国际气压公式 (精度 < 0.1 m 海拔附近,3% 误差到 8000 m)
 *  h = 44330 * (1 - (P/P0)^(1/5.255))  单位 m
 *  返回 0.01 m = 1 cm 单位 */
int32_t BMP280_CalcAltitudeCm(uint32_t pressure_pa, uint32_t sea_level_pa) {
    if (sea_level_pa == 0 || pressure_pa == 0) return 0;
    /* 用 pow 算 (P/P0)^(1/5.255),再换算到 0.01 m */
    double ratio = (double)pressure_pa / (double)sea_level_pa;
    double alt_m = 44330.0 * (1.0 - pow(ratio, 1.0 / 5.255));
    return (int32_t)(alt_m * 100.0);
}

/* ========== 对外 API ========== */

BMP280_Status_t BMP280_ReadID(uint8_t *id) {
    if (id == NULL) return BMP280_ERR_PARAM;
    return BMP280_ReadRegs(BMP280_REG_ID, id, 1);
}

BMP280_Status_t BMP280_ReadStatus(uint8_t *status) {
    if (status == NULL) return BMP280_ERR_PARAM;
    return BMP280_ReadRegs(BMP280_REG_STATUS, status, 1);
}

BMP280_Status_t BMP280_SoftReset(void) {
    BMP280_Status_t r = BMP280_WriteReg(BMP280_REG_RESET, BMP280_SOFT_RESET_CMD);
    if (r == BMP280_OK) {
        BMP280_I2cGet()->delay_ms(BMP280_SOFT_RESET_TIME_MS);
    }
    return r;
}

fsp_err_t BMP280_Init(void) {
    /* 0. 检查 I2C 是否已开 */
    if (i2c_port_get() == NULL) return FSP_ERR_NOT_OPEN;

    /* 1. 读 chip ID */
    BMP280_Status_t r = BMP280_ReadID(&g_chip_id);
    if (r != BMP280_OK) return FSP_ERR_TIMEOUT;
    if (g_chip_id != BMP280_CHIP_ID) {
        return FSP_ERR_UNSUPPORTED;   /* 0x58 != 读到值,挂的不是 BMP280 */
    }

    /* 2. 软复位 + 等待 */
    r = BMP280_SoftReset();
    if (r != BMP280_OK) return FSP_ERR_TIMEOUT;

    /* 3. 读校准系数(必须在 reset 之后立即读,数据手册推荐) */
    r = BMP280_ReadCalibration(&g_calib);
    if (r != BMP280_OK) return FSP_ERR_TIMEOUT;

    /* 4. 配置:osrs_t=x2, osrs_p=x16, mode=SLEEP
     *    不用 NORMAL 模式 → 避免和后续 ReadForced 的 FORCED 模式切换时序冲突
     *    standby=500ms, IIR=4, spi3w=off (这些只在 NORMAL 时才生效,这里只是占位) */
    uint8_t ctrl_meas = BMP280_OSRS_T_x2 | BMP280_OSRS_P_x16 | BMP280_MODE_SLEEP;
    uint8_t config    = BMP280_STANDBY_500_MS | BMP280_FILTER_COEFF_4;

    r = BMP280_WriteReg(BMP280_REG_CTRL_MEAS, ctrl_meas);
    if (r != BMP280_OK) return FSP_ERR_TIMEOUT;
    r = BMP280_WriteReg(BMP280_REG_CONFIG, config);
    if (r != BMP280_OK) return FSP_ERR_TIMEOUT;

    g_initialized = true;
    return FSP_SUCCESS;
}

BMP280_Status_t BMP280_ReadForced(BMP280_Data_t *data) {
    if (data == NULL) return BMP280_ERR_PARAM;
    if (!g_initialized) return BMP280_ERR_I2C;

    const i2c_port_t *p = BMP280_I2cGet();

    /* 1. 触发一次 forced 测量 (写 ctrl_meas, mode=forced) */
    uint8_t ctrl_meas = BMP280_OSRS_T_x2 | BMP280_OSRS_P_x16 | BMP280_MODE_FORCED;
    BMP280_Status_t r = BMP280_WriteReg(BMP280_REG_CTRL_MEAS, ctrl_meas);
    if (r != BMP280_OK) return r;

    /* 2. 等测量完成:轮询 status 寄存器的 bit3 (measuring) */
    uint8_t status = 0;
    int waited_ms = 0;
    while (waited_ms < 50) {   /* 上限 50 ms,osrs_p=x16 典型 10.4 ms */
        r = BMP280_ReadStatus(&status);
        if (r != BMP280_OK) return r;
        if ((status & 0x08) == 0) break;   /* measuring=0 表示完成 */
        p->delay_ms(1);
        waited_ms++;
    }
    if (waited_ms >= 50) return BMP280_ERR_I2C;

    /* 3. 读 6 字节原始数据 (压力 3 + 温度 3) */
    uint8_t raw[6] = {0};
    r = BMP280_ReadRegs(BMP280_REG_PRESS_MSB, raw, 6);
    if (r != BMP280_OK) return r;

    /* 4. 拼装 20-bit ADC 值 (little-endian, MSB 在低地址) */
    int32_t adc_P = ((int32_t)raw[0] << 12) | ((int32_t)raw[1] << 4) | ((int32_t)raw[2] >> 4);
    int32_t adc_T = ((int32_t)raw[3] << 12) | ((int32_t)raw[4] << 4) | ((int32_t)raw[5] >> 4);

    /* 5. 补偿:先 T (算出 t_fine) 再 P */
    int32_t t_fine = 0;
    data->temperature_centi_c = BMP280_CompensateTemperature(adc_T, &t_fine);
    data->pressure_pa         = BMP280_CompensatePressure(adc_P, t_fine);
    data->altitude_cm         = BMP280_CalcAltitudeCm(data->pressure_pa, g_sea_level_pa);

    return BMP280_OK;
}

/* ========== 诊断:打印校准系数 + 一次测量 ========== */
void BMP280_DebugDump(void) {
    printf("[BMP280] chip id: 0x%02X (expect 0x58)\n", g_chip_id);
    printf("  dig_T: %u %d %d\n", g_calib.dig_T1, g_calib.dig_T2, g_calib.dig_T3);
    printf("  dig_P: %u %d %d %d %d %d %d %d %d\n",
           g_calib.dig_P1, g_calib.dig_P2, g_calib.dig_P3,
           g_calib.dig_P4, g_calib.dig_P5, g_calib.dig_P6,
           g_calib.dig_P7, g_calib.dig_P8, g_calib.dig_P9);

    BMP280_Data_t d;
    if (BMP280_ReadForced(&d) == BMP280_OK) {
        printf("  T = %d.%02d C  P = %u Pa  Alt = %d.%02d m\n",
               d.temperature_centi_c / 100,  d.temperature_centi_c % 100,
               d.pressure_pa,
               d.altitude_cm / 100,  d.altitude_cm % 100);
    } else {
        printf("  read failed\n");
    }
}
