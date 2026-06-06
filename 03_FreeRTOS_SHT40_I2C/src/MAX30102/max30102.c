/* V1.0 MAX30102 Driver - 采集 + 原始数据导出 */

#include "max30102.h"
#include <stddef.h>
#include <stdio.h>

/* I2C 单次传输阻塞超时: 100 ms 远大于典型 1 ms 测量/读取 */
#define MAX30102_I2C_TIMEOUT_US  100000U

/* ========== 内部状态 (与 BMP280 风格统一) ========== */
static uint8_t g_chip_id      = 0;
static bool    g_initialized  = false;

/* ========== 内部:取 I2C 平台接口 ========== */
static inline const i2c_port_t * MAX30102_I2cGet(void) {
    return i2c_port_get();
}

/* ========== 内部:写 1 字节寄存器 ========== */
static MAX30102_Status_t MAX30102_WriteReg(uint8_t reg, uint8_t val) {
    const i2c_port_t *p = MAX30102_I2cGet();
    uint8_t buf[2] = { reg, val };
    int err = p->write(MAX30102_I2C_ADDR, buf, 2, false, MAX30102_I2C_TIMEOUT_US);
    return (err == 0) ? MAX30102_OK : MAX30102_ERR_I2C;
}

/* ========== 内部:读 N 字节寄存器 (reg + data) ========== */
static MAX30102_Status_t MAX30102_ReadRegs(uint8_t reg, uint8_t *buf, uint32_t len) {
    const i2c_port_t *p = MAX30102_I2cGet();

    /* 1. 写寄存器地址 (restart=true 保持总线,接着读) */
    int err = p->write(MAX30102_I2C_ADDR, &reg, 1, true, MAX30102_I2C_TIMEOUT_US);
    if (err != 0) return MAX30102_ERR_I2C;

    /* 2. 读 N 字节 (restart=false 末尾发 STOP) */
    err = p->read(MAX30102_I2C_ADDR, buf, len, false, MAX30102_I2C_TIMEOUT_US);
    return (err == 0) ? MAX30102_OK : MAX30102_ERR_I2C;
}

/* ========== 内部:把 3 字节 18-bit ADC 拼成 32-bit 值 ========== */
/*  MAX30102 SpO2 模式:每样本 6 字节 = R[3] + IR[3]
 *  每通道 18-bit 数据是 24-bit 容器的高 18 位,低 6 位补 0 */
static inline uint32_t MAX30102_UnpackAdc(const uint8_t b[3]) {
    return ((uint32_t)b[0] << 16) | ((uint32_t)b[1] << 8) | b[2];
}

/* ========== 对外 API ========== */

MAX30102_Status_t MAX30102_ReadID(uint8_t *id) {
    if (id == NULL) return MAX30102_ERR_PARAM;
    return MAX30102_ReadRegs(MAX30102_REG_PART_ID, id, 1);
}

MAX30102_Status_t MAX30102_ReadFIFOWrPtr(uint8_t *ptr) {
    if (ptr == NULL) return MAX30102_ERR_PARAM;
    return MAX30102_ReadRegs(MAX30102_REG_FIFO_WR_PTR, ptr, 1);
}

MAX30102_Status_t MAX30102_SoftReset(void) {
    /* 写 0x40 到 MODE_CONFIG:bit6=reset,其它位=0 (shdn=0, mode=0) */
    MAX30102_Status_t r = MAX30102_WriteReg(MAX30102_REG_MODE_CONFIG, MAX30102_MODE_RESET);
    if (r != MAX30102_OK) return r;

    /* 复位后等芯片就绪 (数据手册 tRST 典型 1 ms,留 10 ms 余量) */
    MAX30102_I2cGet()->delay_ms(10);
    return MAX30102_OK;
}

fsp_err_t MAX30102_Init(void) {
    if (i2c_port_get() == NULL) return FSP_ERR_NOT_OPEN;

    /* 1. 读 PART_ID (0x15) */
    MAX30102_Status_t r = MAX30102_ReadID(&g_chip_id);
    if (r != MAX30102_OK) return FSP_ERR_TIMEOUT;
    if (g_chip_id != MAX30102_CHIP_ID) {
        return FSP_ERR_UNSUPPORTED;   /* 挂的不是 MAX30102 */
    }

    /* 2. 软复位 */
    r = MAX30102_SoftReset();
    if (r != MAX30102_OK) return FSP_ERR_TIMEOUT;

    /* 3. 清空 FIFO 三个指针(写 0x00) */
    r = MAX30102_WriteReg(MAX30102_REG_FIFO_WR_PTR, 0x00);
    if (r != MAX30102_OK) return FSP_ERR_TIMEOUT;
    r = MAX30102_WriteReg(MAX30102_REG_OVF_COUNTER, 0x00);
    if (r != MAX30102_OK) return FSP_ERR_TIMEOUT;
    r = MAX30102_WriteReg(MAX30102_REG_FIFO_RD_PTR, 0x00);
    if (r != MAX30102_OK) return FSP_ERR_TIMEOUT;

    /* 4. FIFO 配置: sample averaging = 4 (内部 4 采样求平均,降噪),
     *              FIFO rollover = enable (满了覆盖旧数据),
     *              almost full threshold = 17
     *  bit7: SMP_AVE[2], bit[6:5]: SMP_AVE[1:0], bit4: FIFO_ROLLOVER_EN, bit[3:0]: A_FULL[4:0]
     *  SMP_AVE=2 (010) → 4 采样平均; FIFO_ROLLOVER_EN=1; A_FULL=17 (0x11)
     *  0b010_1_10001 = 0x51 */
    r = MAX30102_WriteReg(MAX30102_REG_FIFO_CONFIG, 0x51);
    if (r != MAX30102_OK) return FSP_ERR_TIMEOUT;

    /* 5. MODE: SpO2 模式 (红光 + 红外同时采样) */
    r = MAX30102_WriteReg(MAX30102_REG_MODE_CONFIG, MAX30102_MODE_SPO2);
    if (r != MAX30102_OK) return FSP_ERR_TIMEOUT;

    /* 6. SPO2 配置: ADC 范围 = 4096nA (L=00), 采样率 = 100Hz (SR=001), 脉宽 = 411µs (PW=11)
     *  bit[6:5]: SPO2_ADC_RGE, bit[4:2]: SPO2_SR, bit[1:0]: LED_PW
     *  ADC=4096nA: 00; SR=100Hz: 001; PW=411µs: 11
     *  0b00_001_11 = 0x07 */
    r = MAX30102_WriteReg(MAX30102_REG_SPO2_CONFIG, 0x07);
    if (r != MAX30102_OK) return FSP_ERR_TIMEOUT;

    /* 7. LED 电流: Red = 0x24 (≈ 7.2 mA, 步进 0.2 mA), IR = 0x24
     *  手指血色足时这个值可用;若读数偏小可调到 0x3F (≈ 12.8 mA) */
    r = MAX30102_WriteReg(MAX30102_REG_LED1_PA, 0x24);
    if (r != MAX30102_OK) return FSP_ERR_TIMEOUT;
    r = MAX30102_WriteReg(MAX30102_REG_LED2_PA, 0x24);
    if (r != MAX30102_OK) return FSP_ERR_TIMEOUT;

    /* 8. 中断使能: PPG_RDY (bit6) + A_FULL (bit7) */
    r = MAX30102_WriteReg(MAX30102_REG_INTR_ENABLE_1, 0xC0);
    if (r != MAX30102_OK) return FSP_ERR_TIMEOUT;

    g_initialized = true;
    return FSP_SUCCESS;
}

MAX30102_Status_t MAX30102_ReadFIFO(MAX30102_Sample_t *buf,
                                    uint8_t max_samples,
                                    uint8_t *n_read) {
    if (buf == NULL || n_read == NULL || max_samples == 0) return MAX30102_ERR_PARAM;
    if (!g_initialized) return MAX30102_ERR_I2C;

    *n_read = 0;

    /* 1. 读 WR_PTR / RD_PTR,算可用样本数 */
    uint8_t wr_ptr = 0, rd_ptr = 0;
    MAX30102_Status_t r;

    r = MAX30102_ReadRegs(MAX30102_REG_FIFO_WR_PTR, &wr_ptr, 1);
    if (r != MAX30102_OK) return r;
    r = MAX30102_ReadRegs(MAX30102_REG_FIFO_RD_PTR, &rd_ptr, 1);
    if (r != MAX30102_OK) return r;

    /* FIFO 是环形缓冲: 写入数 = (wr - rd) % 32 */
    uint8_t available = (uint8_t)((wr_ptr - rd_ptr) & 0x1F);
    if (available == 0) return MAX30102_OK;   /* 没新数据,正常 */

    /* 限制读取数,不超出 buffer 也不超出 FIFO 深度 */
    uint8_t to_read = available;
    if (to_read > max_samples) to_read = max_samples;
    if (to_read > MAX30102_FIFO_DEPTH) to_read = MAX30102_FIFO_DEPTH;

    /* 2. 突发读 6 * to_read 字节
     *    - 写 0x87 (FIFO_DATA | 0x80) 触发 auto-increment
     *    - 一次 I2C 事务读 N*6 字节,降低总线开销 */
    uint8_t raw[6 * MAX30102_FIFO_DEPTH] = {0};   /* 最多 32 样本 = 192 字节 */
    const i2c_port_t *p = MAX30102_I2cGet();
    uint8_t reg_burst = MAX30102_FIFO_DATA_BURST;

    int err = p->write(MAX30102_I2C_ADDR, &reg_burst, 1, true, MAX30102_I2C_TIMEOUT_US);
    if (err != 0) return MAX30102_ERR_I2C;
    err = p->read(MAX30102_I2C_ADDR, raw, (uint32_t)to_read * 6, false, MAX30102_I2C_TIMEOUT_US);
    if (err != 0) return MAX30102_ERR_I2C;

    /* 3. 解析:每 6 字节 = [R3, IR3] */
    for (uint8_t i = 0; i < to_read; i++) {
        buf[i].red = MAX30102_UnpackAdc(&raw[i * 6 + 0]);
        buf[i].ir  = MAX30102_UnpackAdc(&raw[i * 6 + 3]);
    }
    *n_read = to_read;
    return MAX30102_OK;
}

/* ========== 诊断:打印 ID + FIFO 状态 + 一次读 ========== */
void MAX30102_DebugDump(void) {
    printf("[MAX30102] chip id: 0x%02X (expect 0x15)\n", g_chip_id);

    uint8_t wr_ptr = 0, rd_ptr = 0, ovf = 0;
    if (MAX30102_ReadFIFOWrPtr(&wr_ptr) == MAX30102_OK) {
        (void)MAX30102_ReadRegs(MAX30102_REG_FIFO_RD_PTR, &rd_ptr, 1);
        (void)MAX30102_ReadRegs(MAX30102_REG_OVF_COUNTER, &ovf, 1);
        printf("  FIFO: wr=%u rd=%u ovf=%u (待读 %u 样本)\n",
               wr_ptr, rd_ptr, ovf, (uint8_t)((wr_ptr - rd_ptr) & 0x1F));
    }

    MAX30102_Sample_t s[4];
    uint8_t got = 0;
    if (MAX30102_ReadFIFO(s, 4, &got) == MAX30102_OK && got > 0) {
        printf("  读到 %u 样本: ", got);
        for (uint8_t i = 0; i < got; i++) {
            printf("[R=%u IR=%u] ", s[i].red, s[i].ir);
        }
        printf("\n");
    } else {
        printf("  读失败或 FIFO 空 (需要把手指贴上传感器)\n");
    }
}
