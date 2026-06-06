#include "spi_screen_thread.h"
#include "Screen/screen.h"
#include "Screen/screen_ui.h"
#include <stdio.h>

/* LED 控制引脚 */
// #define LED_PIN    BSP_IO_PORT_04_PIN_00

void spi_screen_thread_entry(void *pvParameters)
{
    FSP_PARAMETER_NOT_USED (pvParameters);

    uint8_t i = 0;
    TickType_t last_refresh_tick = xTaskGetTickCount();  /* 上一次刷新的 tick 时间戳 */
    TickType_t xLastWakeTime = xTaskGetTickCount();  /* vTaskDelayUntil 基准 */
    const TickType_t xPeriod  = pdMS_TO_TICKS(100);  /* 100 ms 触发周期 */

    while (1) {
        /* 严格 100ms 周期唤醒;若上次任务跑超 100ms,本次立即执行 */
        vTaskDelayUntil(&xLastWakeTime, xPeriod);

        SCREEN_Fill(SCREEN_WHITE);
        UI_DrawTest(i % 5);

        /* 1) SCREEN_Refresh() 返回值:本次刷屏耗时(单位 ms) */
        uint32_t refresh_ms = SCREEN_Refresh();

        /* 2) 用 FreeRTOS tick 算两次刷新之间的间隔(单位 ms) */
        TickType_t now_tick    = xTaskGetTickCount();
        uint32_t interval_ms   = pdTICKS_TO_MS(now_tick - last_refresh_tick);
        last_refresh_tick      = now_tick;

        printf("refresh=%lu ms  interval=%lu ms\r\n", (unsigned long)refresh_ms, (unsigned long)interval_ms);

        i++;
    }
}
