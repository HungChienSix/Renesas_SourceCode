#include "st7735.h"
#include <string.h>
#include "r_spi_api.h"

// 帧缓冲区：128x128 像素, 16位色 (RGB565), 2 字节/像素
static uint16_t display_ram[ST7735_HEIGHT][ST7735_WIDTH] = {0x0000};

#ifdef ST7735_PARTIAL_REFRESH
    static uint16_t page_checksum[ST7735_HEIGHT] = {0x00}; // 页校验码数组
#endif

void SCREEN_RefreshScreen_Force(void) ;

// 辅助时钟
extern uint32_t clock;

// SPI 发送相关变量
uint32_t timeout_us = 100000;
volatile bool spi_transfer_complete_flag = false;

void sci_spi_callback(spi_callback_args_t *p_args){
    switch (p_args->event)
    {
        case SPI_EVENT_TRANSFER_COMPLETE:
        {
            spi_transfer_complete_flag  = true;
            break;
        }
        default:
            break;
    }
}

/* 屏幕状态全局变量定义 */
struSCREEN_state_t screen_state = {0};

void ST7735_Reset(void){
    R_IOPORT_PinWrite(&g_ioport_ctrl, BSP_IO_PORT_05_PIN_11, BSP_IO_LEVEL_LOW); // RES
    R_BSP_SoftwareDelay(100U, BSP_DELAY_UNITS_MILLISECONDS);
	R_IOPORT_PinWrite(&g_ioport_ctrl, BSP_IO_PORT_05_PIN_11, BSP_IO_LEVEL_HIGH); //RES
	R_BSP_SoftwareDelay(100U, BSP_DELAY_UNITS_MILLISECONDS);
}

void ST7735_WriteCmd(uint8_t cmd){
    R_IOPORT_PinWrite(&g_ioport_ctrl, BSP_IO_PORT_08_PIN_04, BSP_IO_LEVEL_LOW);
    R_IOPORT_PinWrite(&g_ioport_ctrl, BSP_IO_PORT_08_PIN_03, BSP_IO_LEVEL_LOW);

    R_SCI_SPI_Write(&g_spi0_ctrl, &cmd, 1, SPI_BIT_WIDTH_8_BITS);

    while ((true != spi_transfer_complete_flag) && timeout_us>0){
        R_BSP_SoftwareDelay(1U, BSP_DELAY_UNITS_MICROSECONDS);
        timeout_us--;
    }
    spi_transfer_complete_flag = false;
    timeout_us           = 100000;

	R_IOPORT_PinWrite(&g_ioport_ctrl, BSP_IO_PORT_08_PIN_03, BSP_IO_LEVEL_HIGH);
}

void ST7735_WriteByte(uint8_t data){
    R_IOPORT_PinWrite(&g_ioport_ctrl, BSP_IO_PORT_08_PIN_04, BSP_IO_LEVEL_HIGH);
    R_IOPORT_PinWrite(&g_ioport_ctrl, BSP_IO_PORT_08_PIN_03, BSP_IO_LEVEL_LOW);

    R_SCI_SPI_Write(&g_spi0_ctrl, &data, 1, SPI_BIT_WIDTH_8_BITS);

    while ((true != spi_transfer_complete_flag) && timeout_us>0){
        R_BSP_SoftwareDelay(1U, BSP_DELAY_UNITS_MICROSECONDS);
        timeout_us--;
    }
    spi_transfer_complete_flag = false;
    timeout_us           = 100000;

    R_IOPORT_PinWrite(&g_ioport_ctrl, BSP_IO_PORT_08_PIN_03, BSP_IO_LEVEL_HIGH);
}

void ST7735_WriteData(uint8_t *data, size_t data_size){
    R_IOPORT_PinWrite(&g_ioport_ctrl, BSP_IO_PORT_08_PIN_04, BSP_IO_LEVEL_HIGH);
    R_IOPORT_PinWrite(&g_ioport_ctrl, BSP_IO_PORT_08_PIN_03, BSP_IO_LEVEL_LOW);

    R_SCI_SPI_Write(&g_spi0_ctrl, data, data_size, SPI_BIT_WIDTH_8_BITS);

    while ((true != spi_transfer_complete_flag) && timeout_us>0){
        R_BSP_SoftwareDelay(1U, BSP_DELAY_UNITS_MICROSECONDS);
        timeout_us--;
    }
    spi_transfer_complete_flag = false;
    timeout_us           = 100000;

	R_IOPORT_PinWrite(&g_ioport_ctrl, BSP_IO_PORT_08_PIN_03, BSP_IO_LEVEL_HIGH);
}

/**
 * @brief 设置屏幕旋转
 * @note  
 */
void ST7735_SetRotation(uint8_t rotation){
	uint8_t madctl = 0;

	switch (rotation)
	{
		case 0:
			madctl = ST7735_MADCTL_MX | ST7735_MADCTL_MY | ST7735_MADCTL_MODE;
		break;
		case 1:
			madctl = ST7735_MADCTL_MY | ST7735_MADCTL_MV | ST7735_MADCTL_MODE;
		break;
		case 2:
			madctl = ST7735_MADCTL_MODE;
		break;
		case 3:
			madctl = ST7735_MADCTL_MX | ST7735_MADCTL_MV | ST7735_MADCTL_MODE;
		break;
	}

	ST7735_WriteCmd(ST7735_MADCTL);
	ST7735_WriteByte(madctl);
}

/**
  * @brief SPI驱动初始化
  * @note
  */
void SPI_ST7735_Init(){
    fsp_err_t err = FSP_SUCCESS;

    err = R_SCI_SPI_Open(&g_spi0_ctrl, &g_spi0_cfg);
    assert(FSP_SUCCESS == err);
}

/**
  * @brief 屏幕初始化
  * @note  
  */
void SCREEN_Init(void) {
    SPI_ST7735_Init();

	ST7735_Reset();
	ST7735_WriteCmd(ST7735_SLPOUT);
	R_BSP_SoftwareDelay(100U, BSP_DELAY_UNITS_MILLISECONDS);
	ST7735_WriteCmd(ST7735_FRMCTR1);
	ST7735_WriteByte(0x01);
	ST7735_WriteByte(0x2C);
	ST7735_WriteByte(0x2D);
	ST7735_WriteCmd(ST7735_FRMCTR2);
	ST7735_WriteByte(0x01);
	ST7735_WriteByte(0x2C);
	ST7735_WriteByte(0x2D);
	ST7735_WriteCmd(ST7735_FRMCTR3);
	ST7735_WriteByte(0x01);
	ST7735_WriteByte(0x2C);
	ST7735_WriteByte(0x2D);
	ST7735_WriteByte(0x01);
	ST7735_WriteByte(0x2C);
	ST7735_WriteByte(0x2D);
	ST7735_WriteCmd(ST7735_INVCTR);
	ST7735_WriteByte(0x07);
	ST7735_WriteCmd(ST7735_PWCTR1);
	ST7735_WriteByte(0xA2);
	ST7735_WriteByte(0x02);
	ST7735_WriteByte(0x84);
	ST7735_WriteCmd(ST7735_PWCTR2);
	ST7735_WriteByte(0xC5);
	ST7735_WriteCmd(ST7735_PWCTR3);
	ST7735_WriteByte(0x0A);
	ST7735_WriteByte(0x00);
	ST7735_WriteCmd(ST7735_PWCTR4);
	ST7735_WriteByte(0x8A);
	ST7735_WriteByte(0x2A);
	ST7735_WriteCmd(ST7735_PWCTR5);
	ST7735_WriteByte(0x8A);
	ST7735_WriteByte(0xEE);
	ST7735_WriteCmd(ST7735_VMCTR1);
	ST7735_WriteByte(0x0E);
	ST7735_WriteCmd(ST7735_INVERSE ? ST7735_INVON : ST7735_INVOFF);
	ST7735_WriteCmd(ST7735_COLMOD);
	ST7735_WriteByte(0x05); // 16-bit color
	ST7735_WriteCmd(ST7735_CASET);
	ST7735_WriteByte(0x00);
	ST7735_WriteByte(0x00);
	ST7735_WriteByte(0x00);
	ST7735_WriteByte(ST7735_HEIGHT - 1); // 0x7F for 128
	ST7735_WriteCmd(ST7735_RASET);
	ST7735_WriteByte(0x00);
	ST7735_WriteByte(0x00);
	ST7735_WriteByte(0x00);
	ST7735_WriteByte(ST7735_HEIGHT - 1); // 0x7F for 128
	ST7735_WriteCmd(ST7735_GMCTRP1);
	ST7735_WriteByte(0x02);
	ST7735_WriteByte(0x1C);
	ST7735_WriteByte(0x07);
	ST7735_WriteByte(0x12);
	ST7735_WriteByte(0x37);
	ST7735_WriteByte(0x32);
	ST7735_WriteByte(0x29);
	ST7735_WriteByte(0x2D);
	ST7735_WriteByte(0x29);
	ST7735_WriteByte(0x25);
	ST7735_WriteByte(0x2B);
	ST7735_WriteByte(0x39);
	ST7735_WriteByte(0x00);
	ST7735_WriteByte(0x01);
	ST7735_WriteByte(0x03);
	ST7735_WriteByte(0x10);
	ST7735_WriteCmd(ST7735_GMCTRN1);
	ST7735_WriteByte(0x03);
	ST7735_WriteByte(0x1D);
	ST7735_WriteByte(0x07);
	ST7735_WriteByte(0x06);
	ST7735_WriteByte(0x2E);
	ST7735_WriteByte(0x2C);
	ST7735_WriteByte(0x29);
	ST7735_WriteByte(0x2D);
	ST7735_WriteByte(0x2E);
	ST7735_WriteByte(0x2E);
	ST7735_WriteByte(0x37);
	ST7735_WriteByte(0x3F);
	ST7735_WriteByte(0x00);
	ST7735_WriteByte(0x00);
	ST7735_WriteByte(0x02);
	ST7735_WriteByte(0x10);
	ST7735_WriteCmd(ST7735_NORON);
	R_BSP_SoftwareDelay(50U, BSP_DELAY_UNITS_MILLISECONDS);
	ST7735_WriteCmd(ST7735_DISPON);
	R_BSP_SoftwareDelay(50U, BSP_DELAY_UNITS_MILLISECONDS);
	ST7735_SetRotation(ST7735_ROTATION);
	FillScreen(SCREEN_BLACK);
	SCREEN_RefreshScreen();
	R_BSP_SoftwareDelay(100U, BSP_DELAY_UNITS_MILLISECONDS);
}

/**
  * @brief 创建绘图区域
  * @note  坐标 (x, y) = (水平, 垂直)
  */
void ST7735_SetAddressWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1){
	uint8_t data[4] = {0x00};
	
	x0 += ST7735_XSTART;
	x1 += ST7735_XSTART;
	y0 += ST7735_YSTART;
	y1 += ST7735_YSTART;
	
	ST7735_WriteCmd(ST7735_CASET); // Column
	data[1] = x0;
	data[3] = x1;
	ST7735_WriteData(data, sizeof(data));

	ST7735_WriteCmd(ST7735_RASET); // Row
	data[1] = y0;
	data[3] = y1;
	ST7735_WriteData(data, sizeof(data));
}

/**
 * @brief 在缓冲区读取像素点状态
 * @param x: X坐标(0-127)
 * @param y: Y坐标(0-127)
 * @return 像素状态 SCREEN_Pixel_t 像素RGB565值
 */
SCREEN_Pixel_t ReadPixel(int16_t x, int16_t y){
	return display_ram[y][x];
}

/**
 * @brief 绘制像素点
 * @param x, y: 坐标
 * @param Pixel_Set: 像素状态
 */
void DrawPixel(int16_t x, int16_t y, SCREEN_Pixel_t Pixel_Set){
	if((x < 0)||(y < 0)||(x > ST7735_WIDTH-1)||(y > ST7735_HEIGHT-1))
		return ;
	display_ram[y][x] = Pixel_Set;
}

/**
 * @brief 绘制水平线 
 */
void DrawHorLine(int16_t x0, int16_t x1, int16_t y, SCREEN_Pixel_t Pixel_Set, SCREEN_Mode_t type){
    uint16_t x_start = (uint8_t) (x0 < x1)? x0 : x1;
    uint16_t x_end   = (uint8_t) (x0 < x1)? x1 : x0;
	
	for(uint16_t x = x_start; x <= x_end; x++) {
		if((type == SCREEN_Xor)&&(ReadPixel(x,y) == Pixel_Set))
			DrawPixel(x, y, ~Pixel_Set);
		else
			DrawPixel(x, y, Pixel_Set);
	}
}

/**
 * @brief 绘制垂直线 
 */
void DrawVerLine(int16_t x, int16_t y0, int16_t y1, SCREEN_Pixel_t Pixel_Set, SCREEN_Mode_t type){
	uint16_t y_start = (uint8_t) (y0 < y1)? y0 : y1;
    uint16_t y_end   = (uint8_t) (y0 < y1)? y1 : y0;
	
	for(int16_t y = y_start; y <= y_end; y++) {
		if((type == SCREEN_Xor)&&(ReadPixel(x,y) == Pixel_Set))
			DrawPixel(x, y, ~Pixel_Set);
		else
			DrawPixel(x, y, Pixel_Set);
	}
}

/**
 * @brief 填充整个屏幕
 */
void FillScreen(SCREEN_Pixel_t Pixel_Set) {
	if(Pixel_Set == SCREEN_BLACK){
		memset(display_ram, Pixel_Set ? 0xFF : 0x00, sizeof(display_ram));
	}
	else if(Pixel_Set == SCREEN_WHITE){
		memset(display_ram, Pixel_Set ? 0xFF : 0x00, sizeof(display_ram));
	}
	else{
	for (uint32_t h = 0; h < ST7735_HEIGHT; h++) {
		for (uint32_t w = 0; w < ST7735_WIDTH; w++) {
	  		display_ram[h][w] = Pixel_Set;
		}
  	}
	}
}

#ifdef ST7735_PARTIAL_REFRESH
/**
 * @brief 计算页数据的校验码
 * @note 使用CRC-16-CCITT算法，工业标准，检测能力强
 *       多项式：0x1021，初始值：0xFFFF，反向输入/输出
 *       查表法优化，速度快且可靠
 */
// CRC-16-CCITT查表（预计算）
static const uint16_t crc16_table[256] = {
    0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50A5, 0x60C6, 0x70E7,
    0x8108, 0x9129, 0xA14A, 0xB16B, 0xC18C, 0xD1AD, 0xE1CE, 0xF1EF,
    0x1231, 0x0210, 0x3273, 0x2252, 0x52B5, 0x4294, 0x72F7, 0x62D6,
    0x9339, 0x8318, 0xB37B, 0xA35A, 0xD3BD, 0xC39C, 0xF3FF, 0xE3DE,
    0x2462, 0x3443, 0x0420, 0x1401, 0x64E6, 0x74C7, 0x44A4, 0x5485,
    0xA56A, 0xB54B, 0x8528, 0x9509, 0xE5EE, 0xF5CF, 0xC5AC, 0xD58D,
    0x3653, 0x2672, 0x1611, 0x0630, 0x76D7, 0x66F6, 0x5695, 0x46B4,
    0xB75B, 0xA77A, 0x9719, 0x8738, 0xF7DF, 0xE7FE, 0xD79D, 0xC7BC,
    0x48C4, 0x58E5, 0x6886, 0x78A7, 0x0840, 0x1861, 0x2802, 0x3823,
    0xC9CC, 0xD9ED, 0xE98E, 0xF9AF, 0x8948, 0x9969, 0xA90A, 0xB92B,
    0x5AF5, 0x4AD4, 0x7AB7, 0x6A96, 0x1A71, 0x0A50, 0x3A33, 0x2A12,
    0xDBFD, 0xCBDC, 0xFBBF, 0xEB9E, 0x9B79, 0x8B58, 0xBB3B, 0xAB1A,
    0x6CA6, 0x7C87, 0x4CE4, 0x5CC5, 0x2C22, 0x3C03, 0x0C60, 0x1C41,
    0xEDAE, 0xFD8F, 0xCDEC, 0xDDCD, 0xAD2A, 0xBD0B, 0x8D68, 0x9D49,
    0x7E97, 0x6EB6, 0x5ED5, 0x4EF4, 0x3E13, 0x2E32, 0x1E51, 0x0E70,
    0xFF9F, 0xEFBE, 0xDFDD, 0xCFFC, 0xBF1B, 0xAF3A, 0x9F59, 0x8F78,
    0x9188, 0x81A9, 0xB1CA, 0xA1EB, 0xD10C, 0xC12D, 0xF14E, 0xE16F,
    0x1080, 0x00A1, 0x30C2, 0x20E3, 0x5004, 0x4025, 0x7046, 0x6067,
    0x83B9, 0x9398, 0xA3FB, 0xB3DA, 0xC33D, 0xD31C, 0xE37F, 0xF35E,
    0x02B1, 0x1290, 0x22F3, 0x32D2, 0x4235, 0x5214, 0x6277, 0x7256,
    0xB5EA, 0xA5CB, 0x95A8, 0x8589, 0xF56E, 0xE54F, 0xD52C, 0xC50D,
    0x34E2, 0x24C3, 0x14A0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
    0xA7DB, 0xB7FA, 0x8799, 0x97B8, 0xE75F, 0xF77E, 0xC71D, 0xD73C,
    0x26D3, 0x36F2, 0x0691, 0x16B0, 0x6657, 0x7676, 0x4615, 0x5634,
    0xD94C, 0xC96D, 0xF90E, 0xE92F, 0x99C8, 0x89E9, 0xB98A, 0xA9AB,
    0x5844, 0x4865, 0x7806, 0x6827, 0x18C0, 0x08E1, 0x3882, 0x28A3,
    0xCB7D, 0xDB5C, 0xEB3F, 0xFB1E, 0x8BF9, 0x9BD8, 0xABBB, 0xBB9A,
    0x4A75, 0x5A54, 0x6A37, 0x7A16, 0x0AF1, 0x1AD0, 0x2AB3, 0x3A92,
    0xFD2E, 0xED0F, 0xDD6C, 0xCD4D, 0xBDAA, 0xAD8B, 0x9DE8, 0x8DC9,
    0x7C26, 0x6C07, 0x5C64, 0x4C45, 0x3CA2, 0x2C83, 0x1CE0, 0x0CC1,
    0xEF1F, 0xFF3E, 0xCF5D, 0xDF7C, 0xAF9B, 0xBFBA, 0x8FD9, 0x9FF8,
    0x6E17, 0x7E36, 0x4E55, 0x5E74, 0x2E93, 0x3EB2, 0x0ED1, 0x1EF0
};

uint16_t SCREEN_CalculateChecksum(uint16_t height) {
    uint16_t crc = 0xFFFF;  // CRC-16初始值

    for (int i = 0; i < ST7735_WIDTH; i++) {
        uint16_t data = display_ram[height][i];

        // 处理高字节
        crc = (crc << 8) ^ crc16_table[((crc >> 8) ^ (data >> 8)) & 0xFF];

        // 处理低字节
        crc = (crc << 8) ^ crc16_table[((crc >> 8) ^ (data & 0xFF)) & 0xFF];
    }

    return crc;  // 返回CRC-16校验码
}
#endif

/**
 * @brief 强制刷新整个屏幕
 */
void SCREEN_RefreshScreen_Force(void) {
    ST7735_SetAddressWindow(0, 0, ST7735_WIDTH - 1, ST7735_HEIGHT - 1);
    ST7735_WriteCmd(ST7735_RAMWR);
    
    static uint8_t buff[ST7735_WIDTH * 2];  // 单行缓冲区
    
    for (uint16_t h = 0; h < ST7735_HEIGHT; h++) {
        // 准备当前行数据
        for (uint16_t w = 0; w < ST7735_WIDTH; w++) {
            buff[w * 2] = (uint8_t)display_ram[h][w] >> 8;
            buff[w * 2 + 1] = (uint8_t)display_ram[h][w] & 0xFF;
        }
        
        // 发送当前行数据
        ST7735_WriteData(buff, ST7735_WIDTH * 2);
    }
}



/**
 * @brief 刷新校验码变化的屏幕
 * @note 时间测量单位：毫秒(ms)
 */
void SCREEN_RefreshScreen(void) {
		static uint32_t last_end_time = 0x00;   // 上次刷新结束时间
		static uint8_t first_call = 1;          // 首次调用标志
		uint32_t current_start_time;            // 本次刷新开始时间
		uint32_t current_end_time;              // 本次刷新结束时间

		// 记录本次刷新开始时间
		current_start_time = clock;

		// 计算刷新间隔（上次刷新结束到本次刷新开始）
		if (!first_call) {
			if (current_start_time >= last_end_time) {
				screen_state.refresh_interval_ms = current_start_time - last_end_time;
			} else {
				// clock溢出情况
				screen_state.refresh_interval_ms = current_start_time + (0xFFFFFFFF - last_end_time);
			}
		} else {
			screen_state.refresh_interval_ms = 0;  // 首次调用，间隔为0
			first_call = 0;
		}

#ifdef ST7735_PARTIAL_REFRESH
    static uint8_t buff[ST7735_WIDTH * 2];

    for (uint16_t h = 0; h < ST7735_HEIGHT; h++) {
        uint16_t current_checksum = SCREEN_CalculateChecksum(h);

        if (current_checksum != page_checksum[h]) {
			ST7735_SetAddressWindow(0, h, ST7735_WIDTH - 1, h);
			ST7735_WriteCmd(ST7735_RAMWR);

            // 准备行数据
            for (uint16_t w = 0; w < ST7735_WIDTH; w++) {
                buff[w * 2] = (uint8_t)(display_ram[h][w] >> 8);
                buff[w * 2 + 1] = (uint8_t)display_ram[h][w] & 0xFF;
            }

            // 发送数据
            ST7735_WriteData(buff, ST7735_WIDTH * 2);

            // 更新校验和
            page_checksum[h] = current_checksum;
		}
    }
#else
    SCREEN_RefreshScreen_Force();
#endif

		// 记录本次刷新结束时间
		current_end_time = clock;

		// 计算刷新耗时（本次刷新结束 - 本次刷新开始）
		if (current_end_time >= current_start_time) {
			screen_state.refresh_time_ms = current_end_time - current_start_time;
		} else {
			// clock溢出情况
			screen_state.refresh_time_ms = current_end_time + (0xFFFFFFFF - current_start_time);
		}

		// 计算刷新率 FPS (Frames Per Second)
		// FPS = 1000ms / 刷新耗时(ms)
		if (screen_state.refresh_time_ms > 0) {
			screen_state.refresh_rate_fps = 1000.0f / (float)screen_state.refresh_time_ms;
		} else {
			screen_state.refresh_rate_fps = 0.0f;  // 耗时为0，无法计算FPS
		}

		// 保存本次刷新结束时间，供下次计算间隔使用
		last_end_time = current_end_time;
		
		
}

uint32_t SCREEN_GetRefreshTime(){
	return screen_state.refresh_time_ms;
}

uint32_t SCREEN_GetRefreshIntervalTime(){
	return screen_state.refresh_interval_ms;
}

float SCREEN_GetFPS(){
	return screen_state.refresh_rate_fps;
}
