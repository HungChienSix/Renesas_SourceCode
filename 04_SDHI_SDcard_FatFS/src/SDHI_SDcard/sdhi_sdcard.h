/* V1.0 SDHI_SDcard */

#ifndef SDHI_SDCARD_H
#define SDHI_SDCARD_H

// CLK --> P413 (SDHI)
// CMD --> P412 (SDHI)
// DAT0--> P411 (SDHI)

#include "hal_data.h"

// #define CARD_INSERT_DETECT 

/* 初始化SD卡 */
void            SDCard_Init(void);

/* 执行SD卡读取测试 */
void            SDCard_Operation(void);

/* SD卡速度测量 */
uint32_t        SDCard_MeasureReadSpeed(void);

#endif /* SDHI_SDCARD_H */
