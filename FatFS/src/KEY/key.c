/* V1.0 KEY */
/* V1.1 KEY: 优化代码结构 */
/* V1.2 KEY: 加入矩阵键盘 */

#include "key.h"
#include "./TIM_Clock/tim_clock.h"

Key_t key_data[KEY_MAX] = 
{
    {KEY_1, BSP_IO_PORT_02_PIN_14, BSP_IO_PORT_09_PIN_07, BSP_IO_LEVEL_HIGH, KEY_State_Idle, 0, 0, false, false, KEY_Event_NULL},
    {KEY_2, BSP_IO_PORT_02_PIN_14, BSP_IO_PORT_03_PIN_11, BSP_IO_LEVEL_HIGH, KEY_State_Idle, 0, 0, false, false, KEY_Event_NULL},
    {KEY_3, BSP_IO_PORT_09_PIN_00, BSP_IO_PORT_09_PIN_07, BSP_IO_LEVEL_HIGH, KEY_State_Idle, 0, 0, false, false, KEY_Event_NULL},
    {KEY_4, BSP_IO_PORT_09_PIN_00, BSP_IO_PORT_03_PIN_11, BSP_IO_LEVEL_HIGH, KEY_State_Idle, 0, 0, false, false, KEY_Event_NULL}
};

bool Key_IsPressed(Key_ID_t key_id)
{
    if (key_id >= KEY_MAX)
    {
        return false;
    }

    bsp_io_port_pin_t pin_out = key_data[key_id].pin_out;  // 列线(输出)
    bsp_io_port_pin_t pin_in = key_data[key_id].pin_in;    // 行线(输入)
    bsp_io_level_t row_level = BSP_IO_LEVEL_HIGH;

    // 1. 将待检测列线设为低电平，其他列线设为高电平
    for (uint8_t i = 0; i < KEY_MAX; i++)
    {
        if (key_data[i].pin_out == pin_out)
        {
            R_IOPORT_PinWrite(&g_ioport_ctrl, key_data[i].pin_out, BSP_IO_LEVEL_LOW);
        }
        else
        {
            R_IOPORT_PinWrite(&g_ioport_ctrl, key_data[i].pin_out, BSP_IO_LEVEL_HIGH);
        }
    }

    R_BSP_SoftwareDelay(5U, BSP_DELAY_UNITS_MICROSECONDS);

    // 2. 读取行线电平
    R_IOPORT_PinRead(&g_ioport_ctrl, pin_in, &row_level);

    // 3. 恢复所有列线为低电平(初始状态)
    for (uint8_t i = 0; i < KEY_MAX; i++)
    {
        R_IOPORT_PinWrite(&g_ioport_ctrl, key_data[i].pin_out, BSP_IO_LEVEL_LOW);
    }

    // 4. 如果行线为低电平，则按键按下
    return (row_level == BSP_IO_LEVEL_LOW);
}

void Key_Scan(void){
    // 从小到大遍历所有按键，看是否所有按键的状态都被清理了
    for(uint8_t i = 0; i < KEY_MAX; i++){
        if(Key_GetEvent(i) != KEY_Event_NULL)
        {
            return;
        }
    }

    uint32_t current_tick = TIM_Clock_GetTime();

    for (uint8_t i = 0; i < KEY_MAX; i++)
    {
        key_data[i].current_state = Key_IsPressed(i);

        /* 状态机处理 */
        switch (key_data[i].state)
        {
            case KEY_State_Idle:
                /* 检测到按下，进入消抖状态 */
                if (key_data[i].current_state == true)
                {
                    key_data[i].state = KEY_State_Press;
                    key_data[i].press_time = current_tick;
                }
                break;

            case KEY_State_Press:
                /* 消抖时间到达，确认是否真的按下 */
                if ((current_tick - key_data[i].press_time) >= KEY_DEBOUNCE_TIME)
                {
                    if (key_data[i].current_state == true)
                    {
                        /* 确认按下 */
                        key_data[i].state = KEY_State_Confirm;
                    }
                    else
                    {
                        /* 误触发，返回空闲 */
                        key_data[i].state = KEY_State_Idle;
                    }
                }
                break;

            case KEY_State_Confirm:
                /* 按键确认按下状态，检测长按 */
                if (key_data[i].current_state == true)
                {
                    /* 保持检测长按，不触发事件 */
                }
                else
                {
                    /* 按键释放，进入释放状态，并判断是短按还是长按 */
                    key_data[i].state = KEY_State_Release;
                    key_data[i].release_time = current_tick;
                }
                break;

            case KEY_State_Release:
                /* 按键释放状态，确认是否是短按或长按 */
                if (key_data[i].current_state == false)
                {
                    if ((current_tick - key_data[i].release_time) >= KEY_DEBOUNCE_TIME)
                    {
                        /* 确认释放，根据按下时长判断是短按还是长按 */
                        uint32_t press_duration = key_data[i].release_time - key_data[i].press_time;

                        if (press_duration < KEY_LONG_PRESS_TIME)
                        {
                            /* 短按 */
                            key_data[i].event = KEY_Event_ShortPress;
                        }
                        else
                        {
                            /* 长按（按下时长≥1000ms） */
                            key_data[i].event = KEY_Event_LongPress;
                        }
                        key_data[i].state = KEY_State_Idle;
                    }
                }
                break;

            default:
                key_data[i].state = KEY_State_Idle;
                break;
        }

        /* 更新上一次状态 */
        key_data[i].last_state = key_data[i].current_state;
    }
}

Key_Event_t Key_GetEvent(Key_ID_t key_id){
    if (key_id >= KEY_MAX)
    {
        return KEY_Event_NULL;
    }
    return key_data[key_id].event;
}

void Key_ClearEvent(Key_ID_t key_id){
    if (key_id < KEY_MAX)
    {
        key_data[key_id].event = KEY_Event_NULL;
    }
}

Key_t Key_GetInfo(Key_ID_t key_id){
    if (key_id >= KEY_MAX)
    {
        Key_t empty = {0};
        return empty;
    }
    return key_data[key_id];
}
