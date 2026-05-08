/* V1.0 System Time Module */

#ifndef SYS_TIME_H
#define SYS_TIME_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 初始化系统时间计数器（DWT周期计数器） */
void            SysTime_Init(void);

/* 获取当前时间戳 */
uint32_t        SysTime_Get_us(void);

uint32_t        SysTime_Get_ms(void);

/* 计算时间差（CPU周期数） */
uint32_t        SysTime_Elapsed_us(uint32_t start, uint32_t end);

#ifdef __cplusplus
}
#endif

#endif /* SYS_TIME_H */
