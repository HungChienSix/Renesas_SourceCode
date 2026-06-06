/* V1.0 MAX30102 Algorithm Layer Implementation */

#include "max30102_algo.h"
#include <string.h>
#include <stdio.h>

/* ========== 内部状态 ========== */
typedef struct {
    uint32_t red_buf[MAX30102_ALGO_BUF_LEN];
    uint32_t ir_buf [MAX30102_ALGO_BUF_LEN];
    uint16_t head;        /* 下一个写位置 */
    uint16_t count;       /* 累计样本数 (≤ BUF_LEN) */
    float    dc_red;      /* DC 慢跟踪,EMA α=0.001 */
    float    dc_ir;
    uint16_t last_peak_idx;   /* 上一个峰位置(用于不应期) */
} algo_state_t;

static algo_state_t g_algo;

/* ========== DC 跟踪(慢速 EMA) ==========
 * 0.999 / 0.001 → 时间常数 ≈ 1000 样本 = 10s
 * 用于过滤掉 AC(心跳脉动),只保留缓慢变化的 DC
 */
#define DC_EMA_ALPHA  0.001f

/* ========== 初始化 ========== */
void MAX30102_AlgoInit(void) {
    memset(&g_algo, 0, sizeof(g_algo));
    g_algo.dc_red = 50000.0f;   /* 合理初值,避免启动时除零 */
    g_algo.dc_ir  = 50000.0f;
    g_algo.last_peak_idx = (uint16_t)(-MAX30102_PEAK_REFRACTORY_SAMPLES);
}

void MAX30102_AlgoReset(void) {
    MAX30102_AlgoInit();
}

/* ========== 喂样本 ========== */
void MAX30102_AlgoPushSample(uint32_t red, uint32_t ir) {
    g_algo.dc_red = (1.0f - DC_EMA_ALPHA) * g_algo.dc_red + DC_EMA_ALPHA * (float)red;
    g_algo.dc_ir  = (1.0f - DC_EMA_ALPHA) * g_algo.dc_ir  + DC_EMA_ALPHA * (float)ir;

    g_algo.red_buf[g_algo.head] = red;
    g_algo.ir_buf [g_algo.head] = ir;
    g_algo.head = (g_algo.head + 1) & MAX30102_ALGO_BUF_MASK;
    if (g_algo.count < MAX30102_ALGO_BUF_LEN) g_algo.count++;
}

/* ========== 内部:按时间顺序取第 i 个样本(支持环形) ========== */
static inline uint32_t sample_at(const uint32_t *buf, uint16_t i) {
    uint16_t start = (g_algo.count < MAX30102_ALGO_BUF_LEN) ? 0 : g_algo.head;
    return buf[(start + i) & MAX30102_ALGO_BUF_MASK];
}

/* ========== 内部:在 buffer 里找红光 AC 信号的波峰 ==========
 * 简单策略: 局部最大 (sample[i-1] < sample[i] > sample[i+1])
 *           + 阈值: AC > 5% DC
 *           + 不应期: 距上一个峰 ≥ 30 样本
 *
 * 返回: 5 秒窗口里找到的峰数(同时通过 last_peak_idx 输出最后一个峰的位置)
 */
static uint16_t find_red_peaks(uint16_t *out_last_peak) {
    uint16_t n_peaks = 0;
    uint16_t last_peak = 0;
    const float ac_thresh = g_algo.dc_red * 0.05f;  /* AC 必须 > 5% DC */

    /* 至少 100 样本(1s)才开始找 */
    if (g_algo.count < 100) {
        if (out_last_peak) *out_last_peak = 0;
        return 0;
    }

    /* 跳过前后 2 样本(没法判局部最大) */
    for (uint16_t i = 2; i < g_algo.count - 1; i++) {
        uint32_t prev = sample_at(g_algo.red_buf, i - 1);
        uint32_t cur  = sample_at(g_algo.red_buf, i);
        uint32_t next = sample_at(g_algo.red_buf, i + 1);

        /* AC 阈值: 减去 DC 估计后的值要 > 5% DC */
        float ac = (float)cur - g_algo.dc_red;
        if (ac < ac_thresh) continue;

        /* 局部最大 */
        if (cur > prev && cur > next) {
            /* 不应期 */
            if ((i - last_peak) >= MAX30102_PEAK_REFRACTORY_SAMPLES) {
                n_peaks++;
                last_peak = i;
            }
        }
    }

    if (out_last_peak) *out_last_peak = last_peak;
    return n_peaks;
}

/* ========== 内部:计算窗口内的 AC/DC 比值 (用于 SpO2) ========== */
static void compute_ac_dc(float *out_ac_red, float *out_dc_red,
                          float *out_ac_ir,  float *out_dc_ir) {
    if (g_algo.count == 0) {
        *out_ac_red = *out_ac_ir = 0.0f;
        *out_dc_red = *out_dc_ir = 1.0f;
        return;
    }

    uint32_t rmin = UINT32_MAX, rmax = 0;
    uint32_t imin = UINT32_MAX, imax = 0;
    uint64_t rsum = 0, isum = 0;

    for (uint16_t i = 0; i < g_algo.count; i++) {
        uint32_t r = sample_at(g_algo.red_buf, i);
        uint32_t v = sample_at(g_algo.ir_buf,  i);
        if (r < rmin) rmin = r;
        if (r > rmax) rmax = r;
        if (v < imin) imin = v;
        if (v > imax) imax = v;
        rsum += r;
        isum += v;
    }

    *out_ac_red = (float)(rmax - rmin);  /* 峰峰值当 AC */
    *out_dc_red = (float)rsum / g_algo.count;
    *out_ac_ir  = (float)(imax - imin);
    *out_dc_ir  = (float)isum / g_algo.count;
}

/* ========== 取 vitals ========== */
void MAX30102_AlgoGetVitals(MAX30102_Vitals_t *out) {
    if (out == NULL) return;
    out->bpm = 0.0f;
    out->spo2 = 0.0f;
    out->r_ratio = 0.0f;
    out->finger_on = false;

    if (g_algo.count < 100) return;   /* 数据不足 1s,无效 */

    /* 1) 手指接触检测: DC 不能太大(否则是 LED 反射饱和,无手指)
     *    且 AC/DC 不能太小(否则没信号或纯噪声) */
    float ac_red, dc_red, ac_ir, dc_ir;
    compute_ac_dc(&ac_red, &dc_red, &ac_ir, &dc_ir);

    if (dc_red < 5000.0f || dc_red > 150000.0f) {
        out->finger_on = false;
        return;
    }
    float ac_dc_ratio_red = ac_red / dc_red;
    if (ac_dc_ratio_red < MAX30102_FINGER_AC_DC_THRESHOLD) {   /* 默认 0.02 */
        out->finger_on = false;
        return;
    }
    out->finger_on = true;

    /* 2) BPM: 在 5s 窗口内数波峰 */
    uint16_t last_peak = 0;
    uint16_t n_peaks = find_red_peaks(&last_peak);
    if (n_peaks >= 2 && last_peak > 0) {
        /* 5s 窗口里 n 个峰,简单换算 */
        float window_sec = (float)MAX30102_ALGO_BUF_LEN / 100.0f;  /* 100Hz 假设 */
        /* 但 g_algo.count 可能 < BUF_LEN,实际窗口 = count / 100 */
        float actual_sec = (float)g_algo.count / 100.0f;
        out->bpm = (float)n_peaks / actual_sec * 60.0f;
    }

    /* 3) SpO2: R = (AC_R/DC_R) / (AC_IR/DC_IR), 经验公式 */
    if (dc_ir > 0.0f && ac_ir > 0.0f) {
        float r_red  = ac_red / dc_red;
        float r_ir   = ac_ir  / dc_ir;
        if (r_ir > 0.001f) {
            out->r_ratio = r_red / r_ir;
            /* 经验公式: SpO2 = A*R² + B*R + C */
            float spo2 = MAX30102_SPO2_A * out->r_ratio * out->r_ratio
                        + MAX30102_SPO2_B * out->r_ratio
                        + MAX30102_SPO2_C;
            if (spo2 < 0.0f)   spo2 = 0.0f;
            if (spo2 > 100.0f) spo2 = 100.0f;
            out->spo2 = spo2;
        }
    }
}

/* ========== 诊断 ========== */
void MAX30102_AlgoDebugDump(void) {
    printf("[MAX30102 ALGO] count=%u/ head=%u  DC_R=%.0f DC_IR=%.0f\n",
           g_algo.count, g_algo.head, g_algo.dc_red, g_algo.dc_ir);

    if (g_algo.count >= 100) {
        float ac_r, dc_r, ac_i, dc_i;
        compute_ac_dc(&ac_r, &dc_r, &ac_i, &dc_i);
        printf("  AC_R=%.0f  AC_IR=%.0f  ratio_R=%.4f  ratio_IR=%.4f\n",
               ac_r, ac_i, ac_r / dc_r, ac_i / dc_i);

        uint16_t last_peak = 0;
        uint16_t n_peaks = find_red_peaks(&last_peak);
        printf("  peaks=%u  last_peak_idx=%u  (refrac=%u)\n",
               n_peaks, last_peak, MAX30102_PEAK_REFRACTORY_SAMPLES);
    }
}
