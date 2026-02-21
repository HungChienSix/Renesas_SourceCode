/* V1.0 TIM_Clock */
/* V1.1 TIM_Clock: 更改变量名称 */

#ifndef TIM_CLOCK_TIM_CLOCK_H_
#define TIM_CLOCK_TIM_CLOCK_H_

#include "hal_data.h"

/* 外部变量声明 */
extern uint32_t g_sys_tick;    // 1ms系统计时时钟

void TIM_Clock_Init(void);
uint32_t TIM_Clock_GetTime(void);

#endif /* TIM_CLOCK_TIM_CLOCK_H_ */
