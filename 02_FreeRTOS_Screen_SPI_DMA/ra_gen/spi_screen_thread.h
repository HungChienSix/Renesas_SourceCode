/* generated thread header file - do not edit */
#ifndef SPI_SCREEN_THREAD_H_
#define SPI_SCREEN_THREAD_H_
#include "bsp_api.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "hal_data.h"
#ifdef __cplusplus
                extern "C" void spi_screen_thread_entry(void * pvParameters);
                #else
extern void spi_screen_thread_entry(void *pvParameters);
#endif
FSP_HEADER
FSP_FOOTER
#endif /* SPI_SCREEN_THREAD_H_ */
