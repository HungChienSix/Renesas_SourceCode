/* V1.0 System Time Module */

#include <sys_time/sys_time.h>
#include "bsp_api.h"
#include "stdio.h"

/* 初始化系统时间计数器（DWT周期计数器） */
void SysTime_Init(void)
{
    /* 使能DWT追踪功能 */
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;

    /* 使能周期计数器 */
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

    /* 复位计数器 */
    DWT->CYCCNT = 0;
}

/* 获取当前时间戳（CPU周期数） */
uint32_t SysTime_Get(void)
{
    return DWT->CYCCNT;
}

/* 计算时间差（CPU周期数） */
uint32_t SysTime_Elapsed(uint32_t start, uint32_t end)
{
    if(end < start)
    {
        /* 处理计数器溢出情况 */
        return (0xFFFFFFFF - start) + end + 1;
    }
    return end - start;
}

/* CPU周期数转微秒 */
uint32_t SysTime_CyclesToUs(uint32_t cycles)
{
    return cycles / (SystemCoreClock / 1000000);
}

/* CPU周期数转毫秒 */
uint32_t SysTime_CyclesToMs(uint32_t cycles)
{
    return cycles / (SystemCoreClock / 1000);
}

/* 获取系统时钟频率(Hz) */
uint32_t SysTime_GetClockHz(void)
{
    return SystemCoreClock;
}

