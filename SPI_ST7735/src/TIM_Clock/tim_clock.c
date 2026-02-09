/* V1.0 TIM_Clock */

/* 辅助计时时钟,每1ms进行一次计数,用来测量屏幕刷新的耗时 */
#include "hal_data.h"
#include "tim_clock.h"

uint32_t clock = 0;

void TIM_Clock_Init(){
    R_GPT_Open(&g_timer0_ctrl, &g_timer0_cfg);
    R_GPT_Start(&g_timer0_ctrl);
}

void TIM_Clock_Callback(timer_callback_args_t *p_args){
    if ( NULL != p_args)
    {
        if(p_args->event == TIMER_EVENT_CYCLE_END ){
            clock++;
        }
    }
}
