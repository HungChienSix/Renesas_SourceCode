/* generated thread header file - do not edit */
#ifndef I2C_SENSOR_THREAD_H_
#define I2C_SENSOR_THREAD_H_
#include "bsp_api.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "hal_data.h"
#ifdef __cplusplus
                extern "C" void i2c_sensor_thread_entry(void * pvParameters);
                #else
extern void i2c_sensor_thread_entry(void *pvParameters);
#endif
FSP_HEADER
FSP_FOOTER
#endif /* I2C_SENSOR_THREAD_H_ */
