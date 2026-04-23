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

/* SDHI异步传输完成标志（在sdhi_callback中设置） */
/* 0=传输中, 1=传输完成, 2=传输错误 */
extern volatile uint32_t g_transfer_complete;

#endif /* SDHI_SDCARD_H */
