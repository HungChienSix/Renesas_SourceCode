/* V1.0 KEY */
/* V1.1 KEY: 优化代码结构 */

#ifndef KEY_KEY_H_
#define KEY_KEY_H_

#include "bsp_api.h"

/* ==================== 按键状态电平定义 ==================== */
#define KEY_PRESS_LEVEL   BSP_IO_LEVEL_LOW     // 按下时的电平
#define KEY_RELEASE_LEVEL BSP_IO_LEVEL_HIGH    // 释放时的电平

/* ==================== 按键时间参数定义 ==================== */
#define KEY_DEBOUNCE_TIME      20    // 消抖时间 (ms)
#define KEY_LONG_PRESS_TIME    800  // 长按判定时间 (ms)

/* ==================== 按键枚举定义 ==================== */
typedef enum
{
    KEY_1 = 0,        // 按键1 (P0_0)
    KEY_MAX           // 按键数量
} Key_ID_t;

/* ==================== 按键事件枚举 ==================== */
typedef enum
{
    KEY_Event_NULL = 0,       // 无事件
    KEY_Event_ShortPress = 1, // 短按事件
    KEY_Event_LongPress = 2   // 长按事件
} Key_Event_t;

/* ==================== 按键状态枚举 ==================== */
typedef enum
{
    KEY_State_Idle = 0,    // 空闲状态
    KEY_State_Press,       // 按下状态（消抖中）
    KEY_State_Confirm,     // 确认按下状态
    KEY_State_Release,     // 释放状态
    KEY_State_Processing   // 处理状态
} Key_State_t;

/* ==================== 按键结构体 ==================== */
typedef struct
{
    Key_ID_t       id;          // 按键ID
    bsp_io_port_pin_t  pin;        // GPIO端口
    bsp_io_level_t level;       // 当前电平
    Key_State_t    state;       // 按键状态
    uint32_t       press_time;  // 按下时间戳
    uint32_t       release_time;// 释放时间戳
    bool           last_state;  // 上次状态
    bool           current_state;// 当前状态
    Key_Event_t    event;       // 按键事件
} Key_t;

/* ==================== 函数声明 ==================== */

/**
 * @brief 按键初始化
 * @note  初始化按键结构体
 */
void Key_Init(void);

/**
 * @brief 按键扫描函数
 * @note  需要在主循环中周期调用
 */
void Key_Scan(void);

/**
 * @brief 获取按键事件
 * @param key_id 按键ID
 * @return 按键事件
 */
Key_Event_t Key_GetEvent(Key_ID_t key_id);

/**
 * @brief 清除按键事件
 * @param key_id 按键ID
 */
void Key_ClearEvent(Key_ID_t key_id);

/**
 * @brief 获取按键结构体信息
 * @param key_id 按键ID
 * @return 按键结构体
 */
Key_t Key_GetInfo(Key_ID_t key_id);

#endif /* KEY_KEY_H_ */
