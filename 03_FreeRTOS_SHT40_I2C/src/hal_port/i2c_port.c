/* V1.0 I2C Porting Layer - Renesas RA Implementation
 *
 * 模式: 02_FreeRTOS_Screen_SPI_DMA/src/hal_port/screen_port.c 的非阻塞实现
 *   - FreeRTOS binary semaphore (Static) + ISR give, 替代自旋等待
 *   - timeout_us = 0          → 非阻塞
 *   - timeout_us = UINT32_MAX → 永久
 *   - 其他                   → 阻塞等待 (ms 向上取整)
 *   - ABORTED 也给信号量 (并设 abort 标志, 供调用方判断错误类别)
 *   - 启动期 / 异常路径: 信号量为 NULL, 退化到原自旋实现
 */

#include "i2c_port.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include <stdio.h>

/* I2C 错误状态标志 (与 FSP 配置的回调函数共享) */
volatile bool g_i2c_abort = false;  // SCI I2C 中止 (无应答 / 仲裁丢失 / 超时)

/* binary semaphore: ISR → 任务同步
 * 任务侧 R_SCI_I2C_Write/Read 之后 xSemaphoreTake(..., timeout);
 * ISR 侧 sci_i2c_master_callback 完成 / 中止时 xSemaphoreGiveFromISR;
 * 关键: 每次传输前必须先 xSemaphoreTake(..., 0) 清空残留 give
 * 注意: 本项目 configSUPPORT_DYNAMIC_ALLOCATION=0, 只能用 Static 版 API */
static StaticSemaphore_t s_i2c_done_sem_buf;
static SemaphoreHandle_t s_i2c_done_sem = NULL;

/* ========== 内部:按 timeout_us 阻塞等信号量 ==========
 * 返回: pdTRUE = 正常拿到 (完成或中止), pdFALSE = 超时
 * 注意: 拿到的瞬间, g_i2c_abort 标志已经反映真实结果 */
static BaseType_t i2c_hal_wait(uint32_t timeout_us) {
    if (s_i2c_done_sem == NULL) return pdTRUE;  /* 未初始化, 跳过等待 */

    /* 计算 tick: 0=非阻塞, UINT32_MAX=永久, 其他 us→ms 向上取整 */
    TickType_t timeout_ticks;
    if (timeout_us == 0U) {
        timeout_ticks = 0;
    } else if (timeout_us == UINT32_MAX) {
        timeout_ticks = portMAX_DELAY;
    } else {
        uint32_t ms = (timeout_us + 999U) / 1000U;
        timeout_ticks = pdMS_TO_TICKS(ms);
        if (timeout_ticks == 0) timeout_ticks = 1;
    }

    return xSemaphoreTake(s_i2c_done_sem, timeout_ticks);
}

/* ========== I2C 写入 (带超时阻塞) - R_SCI_I2C 接口 ========== */
static int i2c_hal_write(uint8_t addr, const uint8_t *data, uint32_t len, bool restart, uint32_t timeout_us) {
    /* 清空残留 give (上一次的回调可能还没被 take 就来了第二次) */
    if (s_i2c_done_sem != NULL) {
        while (xSemaphoreTake(s_i2c_done_sem, 0) == pdTRUE) { }
    }
    g_i2c_abort = false;

    /* 关键: SCI I2C 的 Write/Read 不带 addr, 必须先用 SlaveAddressSet 切换
     * 否则多个从机挂在同一总线上时, 后调用的设备会被前一个的地址 "卡住" */
    fsp_err_t err = R_SCI_I2C_SlaveAddressSet(&g_i2c0_ctrl, addr, I2C_MASTER_ADDR_MODE_7BIT);
    if (err != FSP_SUCCESS) {
        return (int)err;
    }

    err = R_SCI_I2C_Write(&g_i2c0_ctrl, (uint8_t *)data, len, restart);
    if (err != FSP_SUCCESS) {
        return (int)err;
    }

    /* 阻塞 / 非阻塞 / 永久等待, 由 timeout_us 决定 */
    if (s_i2c_done_sem == NULL) {
        /* 信号量未初始化: 退化到原自旋 (启动期或异常路径) */
        uint32_t waited_us = 0;
        while (waited_us < timeout_us) {
            if (g_i2c_abort) break;
            R_BSP_SoftwareDelay(10U, BSP_DELAY_UNITS_MICROSECONDS);
            waited_us += 10;
        }
        return g_i2c_abort ? (int)FSP_ERR_ABORTED : 0;
    }

    if (i2c_hal_wait(timeout_us) != pdTRUE) {
        return (int)FSP_ERR_TIMEOUT;  // 超时: 总线被外部拉死 / 设备未应答
    }
    return g_i2c_abort ? (int)FSP_ERR_ABORTED : 0;
}

/* ========== I2C 读取 (带超时阻塞) - R_SCI_I2C 接口 ========== */
static int i2c_hal_read(uint8_t addr, uint8_t *data, uint32_t len, bool restart, uint32_t timeout_us) {
    if (s_i2c_done_sem != NULL) {
        while (xSemaphoreTake(s_i2c_done_sem, 0) == pdTRUE) { }
    }
    g_i2c_abort = false;

    /* 与 i2c_hal_write 同样的地址切换, 确保多设备共存 */
    fsp_err_t err = R_SCI_I2C_SlaveAddressSet(&g_i2c0_ctrl, addr, I2C_MASTER_ADDR_MODE_7BIT);
    if (err != FSP_SUCCESS) {
        return (int)err;
    }

    err = R_SCI_I2C_Read(&g_i2c0_ctrl, data, len, restart);
    if (err != FSP_SUCCESS) {
        return (int)err;
    }

    if (s_i2c_done_sem == NULL) {
        uint32_t waited_us = 0;
        while (waited_us < timeout_us) {
            if (g_i2c_abort) break;
            R_BSP_SoftwareDelay(10U, BSP_DELAY_UNITS_MICROSECONDS);
            waited_us += 10;
        }
        return g_i2c_abort ? (int)FSP_ERR_ABORTED : 0;
    }

    if (i2c_hal_wait(timeout_us) != pdTRUE) {
        return (int)FSP_ERR_TIMEOUT;
    }
    return g_i2c_abort ? (int)FSP_ERR_ABORTED : 0;
}

/* 毫秒延时 */
static void i2c_hal_delay_ms(uint32_t ms) {
    R_BSP_SoftwareDelay(ms, BSP_DELAY_UNITS_MILLISECONDS);
}

/* 微秒延时 */
static void i2c_hal_delay_us(uint32_t us) {
    R_BSP_SoftwareDelay(us, BSP_DELAY_UNITS_MICROSECONDS);
}

/* I2C 平台接口实例 */
static const i2c_port_t g_i2c_port = {
    .write    = i2c_hal_write,
    .read     = i2c_hal_read,
    .delay_ms = i2c_hal_delay_ms,
    .delay_us = i2c_hal_delay_us,
};

const i2c_port_t * i2c_port_get(void) {
    return &g_i2c_port;
}

/* I2C 回调 (名字必须与 FSP 配置一致: sci_i2c_master_callback)
 * 在 ISR 中 give 信号量, 唤醒等待的传输任务
 * 必须同时处理 ABORTED, 否则从机无应答时主循环会卡死 */
void sci_i2c_master_callback(i2c_master_callback_args_t *p_args) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    switch (p_args->event) {
        case I2C_MASTER_EVENT_TX_COMPLETE:
        case I2C_MASTER_EVENT_RX_COMPLETE:
            if (s_i2c_done_sem != NULL) {
                xSemaphoreGiveFromISR(s_i2c_done_sem, &xHigherPriorityTaskWoken);
                portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
            }
            break;
        case I2C_MASTER_EVENT_ABORTED:
            /* SCI I2C 中止: 设 abort 标志 + 给信号量唤醒任务 */
            g_i2c_abort = true;
            if (s_i2c_done_sem != NULL) {
                xSemaphoreGiveFromISR(s_i2c_done_sem, &xHigherPriorityTaskWoken);
                portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
            }
            break;
        default:
            break;
    }
}

/* 初始化 I2C 硬件平台 (打开 I2C 外设 + 创建信号量) */
int i2c_port_init(void) {
    fsp_err_t err = R_SCI_I2C_Open(&g_i2c0_ctrl, &g_i2c0_cfg);
    if (err != FSP_SUCCESS) {
        return (int)err;
    }
    if (s_i2c_done_sem == NULL) {
        s_i2c_done_sem = xSemaphoreCreateBinaryStatic(&s_i2c_done_sem_buf);
        if (s_i2c_done_sem == NULL) {
            R_SCI_I2C_Close(&g_i2c0_ctrl);
            return (int)FSP_ERR_OUT_OF_MEMORY;
        }
    }
    return 0;
}

/* ========== 诊断工具: I2C 地址扫描 ==========
 * 注意: 扫描过程用底层 R_SCI_I2C 接口直接探测,
 * 保持 5ms 内必出结果, 不被信号量超时逻辑污染 */
int i2c_port_scan(void) {
    int found = 0;
    printf("[I2C SCAN] start 0x03~0x77\n");

    for (uint8_t addr = 0x03; addr <= 0x77; addr++) {
        fsp_err_t set_err = R_SCI_I2C_SlaveAddressSet(&g_i2c0_ctrl, addr, I2C_MASTER_ADDR_MODE_7BIT);
        if (set_err != FSP_SUCCESS) {
            printf("  setaddr err @0x%02X: %d (skipped)\n", addr, (int)set_err);
            R_BSP_SoftwareDelay(500U, BSP_DELAY_UNITS_MICROSECONDS);
            continue;
        }

        /* 清信号量残留 + 标志, 写 1 字节触发地址探测 */
        if (s_i2c_done_sem != NULL) {
            while (xSemaphoreTake(s_i2c_done_sem, 0) == pdTRUE) { }
        }
        g_i2c_abort = false;

        uint8_t dummy = 0x00;
        fsp_err_t err = R_SCI_I2C_Write(&g_i2c0_ctrl, &dummy, 1, false);
        if (err != FSP_SUCCESS) {
            R_BSP_SoftwareDelay(500U, BSP_DELAY_UNITS_MICROSECONDS);
            continue;
        }

        /* 等回调 (5 ms 内必有结果: 成功 / 中止) */
        BaseType_t got = pdFALSE;
        if (s_i2c_done_sem != NULL) {
            got = xSemaphoreTake(s_i2c_done_sem, pdMS_TO_TICKS(5));
        } else {
            uint32_t waited = 0;
            while (waited < 5000U) {
                if (g_i2c_abort) break;
                R_BSP_SoftwareDelay(10U, BSP_DELAY_UNITS_MICROSECONDS);
                waited += 10;
            }
            got = pdTRUE;
        }

        if (got == pdTRUE && !g_i2c_abort) {
            printf("  0x%02X: ACK\n", addr);
            found++;
        }

        R_BSP_SoftwareDelay(200U, BSP_DELAY_UNITS_MICROSECONDS);
    }

    /* 恢复默认从机地址 0x44 (SHT40) - 实际地址由各 sensor 初始化时再次设置 */
    R_SCI_I2C_SlaveAddressSet(&g_i2c0_ctrl, 0x44, I2C_MASTER_ADDR_MODE_7BIT);

    printf("[I2C SCAN] done, %d device(s) found\n", found);
    return found;
}
