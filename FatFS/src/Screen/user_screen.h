#ifndef SCREEN_USER_SCREEN_H_
#define SCREEN_USER_SCREEN_H_

#include "screen_ui.h"
#include "../sys_info.h"

void LCD_Test(uint8_t test_id);
void Page0_Welcome(void);
void Page1_Main(void);
void Page2_SongInfo(void);

#endif /* SCREEN_USER_SCREEN_H_ */
