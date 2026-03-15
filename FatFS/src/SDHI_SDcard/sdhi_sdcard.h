/* V1.0 SDHI_SDcard */

#ifndef __BSP_SDCARD_H
#define __BSP_SDCARD_H
#include "hal_data.h"

void SDCard_Init(void);

void SDCard_Test(void);
uint32_t SDCard_MeasureReadSpeed(void);

#endif
