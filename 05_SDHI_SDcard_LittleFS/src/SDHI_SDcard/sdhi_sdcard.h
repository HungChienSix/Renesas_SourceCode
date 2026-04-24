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

/* 读取SD卡 */
uint8_t         SDCard_Read(uint8_t *buff, uint32_t sector, uint32_t count, uint32_t timeout_us);

/* 写入SD卡 */
uint8_t         SDCard_Write(const uint8_t *buff, uint32_t sector, uint32_t count, uint32_t timeout_us);

/* IO控制 */
void            SDCard_ioctl(uint8_t cmd, void *buff);

/* 执行SD卡读取测试 */
void            SDCard_Operation(void);

/* SD卡速度测量 */
uint32_t        SDCard_MeasureReadSpeed(void);

#endif /* SDHI_SDCARD_H */
