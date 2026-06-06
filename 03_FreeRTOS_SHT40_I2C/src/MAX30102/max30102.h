/* V1.0 MAX30102 Heart Rate / SpO2 Sensor Driver
 *
 * Maxim MAX30102: 反射式 PPG(光电容积脉搏波)传感器
 *  - 内置红光(660nm)+ 红外(880nm)LED
 *  - 18-bit ADC,FIFO 深度 32
 *  - 7-bit I2C 地址 0x57
 *  - 数据手册 Rev 2 (19-7549)
 *
 * 本驱动只做 "采集 + 原始数据导出"。
 * 心率 / SpO2 推导需要后续 PPG 算法(峰值检测 + 红/外比值查表),
 * 不在本驱动范围内。
 *
 * I2C 访问走通用 hal_port/i2c_port,与 SHT40 / BMP280 共用同一总线。
 */

#ifndef MAX30102_H
#define MAX30102_H

#include "hal_data.h"
#include "hal_port/i2c_port.h"
#include <stdint.h>
#include <stdbool.h>

/* ========== 器件地址与 ID ========== */
#define MAX30102_I2C_ADDR          0x57
#define MAX30102_CHIP_ID           0x15    /* PART_ID reg 0xFF 应返回此值 */
#define MAX30102_FIFO_DEPTH        32      /* 内部 FIFO 最大 32 个样本 */

/* ========== 寄存器地址 ========== */
#define MAX30102_REG_INTR_STATUS_1 0x00    /* 中断状态 1 (A_FULL, PPG_RDY) */
#define MAX30102_REG_INTR_STATUS_2 0x01    /* 中断状态 2 (DIE_TEMP_RDY, ...) */
#define MAX30102_REG_INTR_ENABLE_1 0x02
#define MAX30102_REG_INTR_ENABLE_2 0x03
#define MAX30102_REG_FIFO_WR_PTR   0x04    /* 写指针,新数据写在哪 */
#define MAX30102_REG_OVF_COUNTER   0x05    /* 溢出计数 */
#define MAX30102_REG_FIFO_RD_PTR   0x06    /* 读指针 */
#define MAX30102_REG_FIFO_DATA     0x07    /* 突发读: 写 0x87 触发 auto-increment */
#define MAX30102_REG_FIFO_CONFIG   0x08
#define MAX30102_REG_MODE_CONFIG   0x09
#define MAX30102_REG_SPO2_CONFIG   0x0A
#define MAX30102_REG_LED1_PA       0x0C    /* 红光 LED 电流 */
#define MAX30102_REG_LED2_PA       0x0D    /* 红外 LED 电流 */
#define MAX30102_REG_PART_ID       0xFF

/* FIFO burst read: MSB 置 1 触发 auto-increment */
#define MAX30102_FIFO_DATA_BURST   (MAX30102_REG_FIFO_DATA | 0x80)

/* ========== 复位 / 模式位 ========== */
#define MAX30102_MODE_SHDN         (1 << 7)  /* 软关断 */
#define MAX30102_MODE_RESET        (1 << 6)  /* 软复位 */
#define MAX30102_MODE_HR           (0x02)    /* 仅红光(心率) */
#define MAX30102_MODE_SPO2         (0x03)    /* 红光 + 红外(血氧) */

/* ========== 驱动状态码 (与 SHT40 / BMP280 命名对齐) ========== */
typedef enum {
    MAX30102_OK         =  0,    /* 成功 */
    MAX30102_ERR_I2C    = -1,    /* I2C 通信失败 */
    MAX30102_ERR_ID     = -2,    /* 读到的 ID != 0x15 */
    MAX30102_ERR_PARAM  = -3,    /* 参数错误 */
    MAX30102_ERR_FIFO   = -4,    /* FIFO 异常 (overflow 等) */
} MAX30102_Status_t;

/* ========== 单个样本 (红 + 红外的 ADC 原始值) ========== */
typedef struct {
    uint32_t red;   /* 18-bit 红光 ADC,左对齐(取高 18 位,低 6 位补 0) */
    uint32_t ir;    /* 18-bit 红外 ADC,同上 */
} MAX30102_Sample_t;

/* ========== 对外 API (与 SHT40 / BMP280 风格统一) ========== */

/**
 * @brief 一次性完成 MAX30102 初始化:读 ID → 复位 → 清 FIFO → 配置
 * @return FSP_SUCCESS 或 FSP_ERR_*
 */
fsp_err_t MAX30102_Init(void);

/**
 * @brief 软复位(写 0x40 到 MODE_CONFIG,等待复位完成)
 */
MAX30102_Status_t MAX30102_SoftReset(void);

/**
 * @brief 读 PART_ID (调试用,应返回 0x15)
 */
MAX30102_Status_t MAX30102_ReadID(uint8_t *id);

/**
 * @brief 读 FIFO 全部可用样本(自动从 RD_PTR 抽到 WR_PTR)
 * @param buf         输出缓冲区
 * @param max_samples buf 容量(单位: 样本,即 sizeof(MAX30102_Sample_t) 个数)
 * @param n_read      实际读到的样本数
 * @return  MAX30102_OK / MAX30102_ERR_PARAM / MAX30102_ERR_I2C
 */
MAX30102_Status_t MAX30102_ReadFIFO(MAX30102_Sample_t *buf,
                                    uint8_t max_samples,
                                    uint8_t *n_read);

/**
 * @brief 读 FIFO 写指针 (调试用)
 */
MAX30102_Status_t MAX30102_ReadFIFOWrPtr(uint8_t *ptr);

/**
 * @brief 诊断: 打印 ID + FIFO 状态 + 一次采样
 */
void MAX30102_DebugDump(void);

#endif /* MAX30102_H */
