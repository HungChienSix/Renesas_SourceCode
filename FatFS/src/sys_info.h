#ifndef SYS_INFO_H_
#define SYS_INFO_H_

#include "I2S/i2s.h"

/**
 * @brief 系统信息结构体
 */
typedef struct {
    struAudio_t *selected_audio;  // 当前选中的音频
    uint8_t      page_id;          // 当前页码（用于分页）
    uint8_t      songs_per_page;    // 每页显示的歌曲数量
    uint8_t      state;       
} sys_info;

/**
 * @brief 全局系统信息变量
 */
extern sys_info g_sys_info;

#endif /* SYS_INFO_H_ */
