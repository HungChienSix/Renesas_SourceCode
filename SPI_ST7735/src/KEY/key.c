/* V1.0 KEY */
/* V1.1 KEY: 优化代码结构 */

#include "key.h"
#include "./TIM_Clock/tim_clock.h"

/* ==================== 按键配置表 ==================== */
static const Key_t key_config[KEY_MAX] =
{
    {KEY_1, BSP_IO_PORT_00_PIN_00, KEY_RELEASE_LEVEL, KEY_State_Idle, 0, 0, false, false, KEY_Event_NULL},
};

/* ==================== 按键运行时数据 ==================== */
static Key_t key_data[KEY_MAX];

/* ==================== 按键初始化 ==================== */
void Key_Init(void)
{
    /* 初始化结构体 */
    for (uint8_t i = 0; i < KEY_MAX; i++)
    {
        key_data[i] = key_config[i];
        key_data[i].event = KEY_Event_NULL;
        key_data[i].state = KEY_State_Idle;
        key_data[i].last_state = false;
        key_data[i].current_state = false;
    }
}

/* ==================== 检测按键是否按下 ==================== */
bool Key_IsPressed(Key_ID_t key_id)
{
    if (key_id >= KEY_MAX)
    {
        return false;
    }
    bsp_io_level_t pin_level = BSP_IO_LEVEL_LOW;
    R_IOPORT_PinRead(&g_ioport_ctrl, key_data[key_id].pin, &pin_level);
    return (pin_level == KEY_PRESS_LEVEL);
}

/* ==================== 按键扫描函数 ==================== */
void Key_Scan(void)
{
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

/* ==================== 获取按键事件 ==================== */
Key_Event_t Key_GetEvent(Key_ID_t key_id)
{
    if (key_id >= KEY_MAX)
    {
        return KEY_Event_NULL;
    }
    return key_data[key_id].event;
}

/* ==================== 清除按键事件 ==================== */
void Key_ClearEvent(Key_ID_t key_id)
{
    if (key_id < KEY_MAX)
    {
        key_data[key_id].event = KEY_Event_NULL;
    }
}

/* ==================== 获取按键结构体 ==================== */
Key_t Key_GetInfo(Key_ID_t key_id)
{
    if (key_id >= KEY_MAX)
    {
        Key_t empty = {0};
        return empty;
    }
    return key_data[key_id];
}
