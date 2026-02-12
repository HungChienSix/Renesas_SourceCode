/* V1.0 TIM_Clock */
/* V1.1 TIM_Clock: 更改变量名称 */

/* 辅助计时时钟,每1ms进行一次计数,用来测量屏幕刷新的耗时 */
#include "tim_clock.h"
#include "./KEY/key.h"

uint32_t g_sys_tick = 0;  // 1ms系统计时时钟

void TIM_Clock_Init(){
    R_GPT_Open(&g_timer0_ctrl, &g_timer0_cfg);
    R_GPT_Start(&g_timer0_ctrl);
}

void TIM_Clock_Callback(timer_callback_args_t *p_args){
    if ( NULL != p_args)
    {
        if(p_args->event == TIMER_EVENT_CYCLE_END ){
            g_sys_tick++;

            Key_Scan();
        }
    }
}
