#include <uart_debug_thread.h>
#include "UART_debug/uart_debug.h"
/* New Thread entry function */
/* pvParameters contains TaskHandle_t */
void uart_debug_thread_entry(void *pvParameters)
{
    FSP_PARAMETER_NOT_USED (pvParameters);

    UART_debug_Init();

    char para[32], value[32];
    while (1)
    {
        if (UART_debug_GetCmd(para, value)) {
            printf("参数: %s, 值: %s\n", para, value);
        }
        vTaskDelay(100);
    }
}
