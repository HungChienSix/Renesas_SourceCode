/* V1.0 MAX30102 Algorithm Layer - 心率 + SpO₂ 推导
 *
 * 这一层把 MAX30102 给的原始红/红外 ADC 数字,转成有用的生理指标:
 *   - 手指接触检测
 *   - 心率 BPM(峰值检测 + 间隔平均)
 *   - 血氧 SpO2(红/红外 AC/DC 比值,经验公式)
 *
 * 喂入: MAX30102_Sample_t 的 red/ir
 * 输出: MAX30102_Vitals_t 的 bpm/spo2/finger_on
 *
 * 算法不复杂但参数调起来很微妙,先跑通看现象再调。
 *
 * 参考资料:
 *   - Maxim MAX30102 Application Note (AN6409, AN6593)
 *   - TI 血氧笔记 SLAA733
 */

#ifndef MAX30102_ALGO_H
#define MAX30102_ALGO_H

#include <stdint.h>
#include <stdbool.h>

/* ========== 5 秒滑动窗口(100Hz 采样 = 500 样本) ========== */
#define MAX30102_ALGO_BUF_LEN  500
#define MAX30102_ALGO_BUF_MASK (MAX30102_ALGO_BUF_LEN - 1)

/* 峰检测后不应期(300ms),避免一个波峰被检测两次 */
#define MAX30102_PEAK_REFRACTORY_SAMPLES 30

/* AC / DC 比值低于这个数认为没信号(手指没贴 / 接触不良)
 * 实测没手指时噪声 AC/DC 通常 < 1%,贴稳的手指 > 2% */
#define MAX30102_FINGER_AC_DC_THRESHOLD  0.02f

/* 经验公式系数(Maxim 文档典型值) */
#define MAX30102_SPO2_A  1.5958422f
#define MAX30102_SPO2_B -34.659662f
#define MAX30102_SPO2_C  112.68987f

/* ========== 输出 ========== */
typedef struct {
    float  bpm;          /* 心率(次/分钟), 0 = 无效 */
    float  spo2;         /* 血氧饱和度(%), 0 = 无效 */
    float  r_ratio;      /* R 值 (AC_R/DC_R) / (AC_IR/DC_IR) */
    bool   finger_on;    /* true = 检测到有效信号 */
} MAX30102_Vitals_t;

/* ========== API ========== */

/**
 * @brief 初始化算法状态(清零 buffer,重置 DC 估计)
 */
void MAX30102_AlgoInit(void);

/**
 * @brief 喂入一个红/红外样本(由 ReadFIFO 回调驱动)
 * @param red  红光 ADC
 * @param ir   红外 ADC
 */
void MAX30102_AlgoPushSample(uint32_t red, uint32_t ir);

/**
 * @brief 取当前 vitals(每调用一次,内部重算一次)
 */
void MAX30102_AlgoGetVitals(MAX30102_Vitals_t *out);

/**
 * @brief 重置(手指挪开又放上时调)
 */
void MAX30102_AlgoReset(void);

/**
 * @brief 诊断: 打印当前 buffer 的统计信息
 */
void MAX30102_AlgoDebugDump(void);

#endif /* MAX30102_ALGO_H */
