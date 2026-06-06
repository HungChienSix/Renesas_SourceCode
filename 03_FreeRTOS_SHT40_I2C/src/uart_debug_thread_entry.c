#include "uart_debug_thread.h"
#include "UART_debug/uart_debug.h"
//#include "Screen/screen.h"
#include "SHT40/sht40.h"
#include <stdio.h>

/* Debug Thread entry function */
/* pvParameters contains TaskHandle_t */
void uart_debug_thread_entry(void *pvParameters)
{
    FSP_PARAMETER_NOT_USED (pvParameters);

    UART_debug_Init();

    // if (SCREEN_Init() != FSP_SUCCESS) {
    //     printf("Screen init failed\n");
    // }

    // if (SHT40_Init() != FSP_SUCCESS) {
    //     printf("Sensor init failed\n");
    // }

    char para[32], value[32];
    while (1)
    {
        if (UART_debug_GetCmd(para, value)) {
            printf("参数: %s, 值: %s\n", para, value);
        }
        vTaskDelay(500);
    }
}
