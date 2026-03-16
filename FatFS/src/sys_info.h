#ifndef SYS_INFO_H_
#define SYS_INFO_H_

#include "I2S/i2s.h"
#include "KEY/key.h"

/**
 * @brief 系统信息结构体
 */
typedef struct {
    struAudio_t     *selected_audio;  // 当前选中的音频
    uint8_t         page;          // 当前页码（用于分页）
    bool            is_play;    
    Input_Event_t   input;
} sys_info;

/**
 * @brief 全局系统信息变量
 */
extern sys_info g_sys_info;

#endif /* SYS_INFO_H_ */
