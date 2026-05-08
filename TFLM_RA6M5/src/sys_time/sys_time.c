/* V1.0 System Time Module */

#include "sys_time/sys_time.h"
#include "bsp_api.h"

/* 初始化系统时间计数器（DWT周期计数器） */
void SysTime_Init(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    DWT->CYCCNT = 0;
}

/* 获取当前时间戳（微秒） */
uint32_t SysTime_Get_us(void)
{
    return DWT->CYCCNT / (SystemCoreClock / 1000000);
}

uint32_t SysTime_Get_ms(void)
{
    return DWT->CYCCNT / (SystemCoreClock / 1000);
}

/* 计算时间差 */
uint32_t SysTime_Elapsed_us(uint32_t start, uint32_t end)
{
    if(end < start)
    {
        return (0xFFFFFFFF - start) + end + 1;
    }

    return end - start;
}

