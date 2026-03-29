/* V1.0 System Time Module */

#ifndef __SYS_TIME_H
#define __SYS_TIME_H

#include <stdint.h>

/* 初始化系统时间计数器（DWT周期计数器） */
void SysTime_Init(void);

/* 获取当前时间戳（CPU周期数） */
uint32_t SysTime_Get(void);

/* 计算时间差（CPU周期数） */
uint32_t SysTime_Elapsed(uint32_t start, uint32_t end);

/* CPU周期数转微秒 */
uint32_t SysTime_CyclesToUs(uint32_t cycles);

/* CPU周期数转毫秒 */
uint32_t SysTime_CyclesToMs(uint32_t cycles);

/* 获取系统时钟频率(Hz) */
uint32_t SysTime_GetClockHz(void);

/* 时间测量宏 - 方便使用 */
#define TIME_MEASURE_START()     uint32_t _t_start = SysTime_Get()
#define TIME_MEASURE_END(name)   printf("[%s] 耗时: %d us\r\n", (name), \
                                      SysTime_CyclesToUs(SysTime_Get() - _t_start))

#endif /* __SYS_TIME_H */
