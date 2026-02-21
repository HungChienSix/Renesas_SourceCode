/* V1.0 KEY */
/* V1.1 KEY: 优化代码结构 */
/* V1.2 KEY: 加入矩阵键盘 */

#ifndef KEY_KEY_H_
#define KEY_KEY_H_

#include "bsp_api.h"

// P907 R1 (input) pull-up
// P311 R2 (input) pull-up
// P900 L2 (output) initially low
// P214 L1 (output) initially low

#define KEY_DEBOUNCE_TIME   20    // 消抖时间 (ms)
#define KEY_LONG_PRESS_TIME 800  // 长按判定时间 (ms)

typedef enum{
    KEY_1 = 0,                  
    KEY_2,
    KEY_3,
    KEY_4,
    KEY_MAX                     // 按键数量
} Key_ID_t;

typedef enum{
    KEY_Event_NULL = 0,         // 无事件
    KEY_Event_ShortPress = 1,   // 短按事件
    KEY_Event_LongPress = 2     // 长按事件
} Key_Event_t;

typedef enum{
    KEY_State_Idle = 0,         // 空闲状态
    KEY_State_Press,            // 按下状态（消抖中）
    KEY_State_Confirm,          // 确认按下状态
    KEY_State_Release,          // 释放状态
    KEY_State_Processing        // 处理状态
} Key_State_t;

typedef struct{
    Key_ID_t            id;             // 按键ID
    bsp_io_port_pin_t   pin_out;        // GPIO输出行(列)端口
    bsp_io_port_pin_t   pin_in;         // GPIO输入列(行)端口
    Key_State_t         state;          // 按键状态
    uint32_t            press_time;     // 按下时间戳
    uint32_t            release_time;   // 释放时间戳
    bool                last_state;     // 上次状态
    bool                current_state;  // 当前状态
    Key_Event_t         event;          // 按键事件
} Key_t;


void        Key_Scan(void);
Key_Event_t Key_GetEvent(Key_ID_t key_id);
void        Key_ClearEvent(Key_ID_t key_id);
Key_t       Key_GetInfo(Key_ID_t key_id);

#endif /* KEY_KEY_H_ */
