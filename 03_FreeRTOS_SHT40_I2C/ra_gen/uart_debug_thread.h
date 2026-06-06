/* generated thread header file - do not edit */
#ifndef UART_DEBUG_THREAD_H_
#define UART_DEBUG_THREAD_H_
#include "bsp_api.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "hal_data.h"
#ifdef __cplusplus
                extern "C" void uart_debug_thread_entry(void * pvParameters);
                #else
extern void uart_debug_thread_entry(void *pvParameters);
#endif
FSP_HEADER
FSP_FOOTER
#endif /* UART_DEBUG_THREAD_H_ */
