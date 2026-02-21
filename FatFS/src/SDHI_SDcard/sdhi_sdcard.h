/* V1.0 SDHI_SDcard */

#ifndef __BSP_SDCARD_H
#define __BSP_SDCARD_H
#include "hal_data.h"

void SDCard_Init(void);
void SDCard_DeInit(void);

void SDCard_Test(void);

/**
 * @brief 测量SD卡文件读取速度
 * @param filepath 文件路径（如 "1:test.wav"）
 * @param buffer_size 每次读取的缓冲区大小（字节），0表示一次性读取整个文件
 * @return 读取速度（KB/s），失败返回0
 */
uint32_t SDCard_MeasureReadSpeed(const char *filepath, uint32_t buffer_size);


#endif
