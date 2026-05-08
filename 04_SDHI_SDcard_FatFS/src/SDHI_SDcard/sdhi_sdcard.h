/* V1.0 SDHI_SDcard */

#ifndef SDHI_SDCARD_H
#define SDHI_SDCARD_H

// CLK --> P413 (SDHI)
// CMD --> P412 (SDHI)
// DAT0--> P411 (SDHI)

#include "hal_data.h"

#define SD_CARD_ENABLE      1    

#define CARD_INSERT_DETECT_ENABLE   0    
#define SDHI_SDCARD_CTRL    g_sdmmc0_ctrl
#define SDHI_SDCARD_CFG     g_sdmmc0_cfg

typedef enum SDCard_ERROR {
    SDHI_OK = 0,
    SDHI_Read_Err ,
    SDHI_Read_Timeout ,
    SDHI_Write_Err ,
    SDHI_Write_Timeout
} SDCard_ERROR_t;

/* 初始化SD卡 */
fsp_err_t       SDCard_Init(void);

/* 读取SD卡 */
SDCard_ERROR_t  SDCard_Read(uint8_t *buff, uint32_t sector, uint32_t count, uint32_t timeout_us);

/* 写入SD卡 */
SDCard_ERROR_t  SDCard_Write(const uint8_t *buff, uint32_t sector, uint32_t count, uint32_t timeout_us);

/* IO控制 */
void            SDCard_ioctl(uint8_t cmd, void *buff);

/* SD卡速度测量 */
uint32_t        SDCard_MeasureReadSpeed(void);

#endif /* SDHI_SDCARD_H */
