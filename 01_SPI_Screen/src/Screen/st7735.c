#include "st7735.h"
#include "../sys_time/sys_time.h"

// 帧缓冲区：128x128 像素, 16位色 (RGB565), 2 字节/像素
static uint16_t display_ram[ST7735_HEIGHT][ST7735_WIDTH] = {0x0000};

#ifdef ST7735_PARTIAL_REFRESH
    // 列范围追踪：记录每行需要刷新的最小/最大列
    static uint8_t min_col[ST7735_HEIGHT];  // 每行最小修改列
    static uint8_t max_col[ST7735_HEIGHT];  // 每行最大修改列
    static bool dirty_row[ST7735_HEIGHT];   // 行是否需要刷新
#endif

void SCREEN_RefreshScreen_Force(void) ;

// SPI 发送相关变量
static uint32_t timeout_us ;
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

void ST7735_Reset(void){
    R_IOPORT_PinWrite(&g_ioport_ctrl, TFT_RES_Pin, BSP_IO_LEVEL_LOW); // RES
    R_BSP_SoftwareDelay(100U, BSP_DELAY_UNITS_MILLISECONDS);
	R_IOPORT_PinWrite(&g_ioport_ctrl, TFT_RES_Pin, BSP_IO_LEVEL_HIGH); //RES
	R_BSP_SoftwareDelay(100U, BSP_DELAY_UNITS_MILLISECONDS);
}

void ST7735_WriteCmd(uint8_t cmd){
    R_IOPORT_PinWrite(&g_ioport_ctrl, TFT_DC_Pin, BSP_IO_LEVEL_LOW);
    R_IOPORT_PinWrite(&g_ioport_ctrl, TFT_CS_Pin, BSP_IO_LEVEL_LOW);

    R_SCI_SPI_Write(&g_spi0_ctrl, &cmd, 1, SPI_BIT_WIDTH_8_BITS);

	/* 添加超时机制，避免无限等待 */
	timeout_us = SPI_CMD_TIMEOUT_us;

	while (spi_transfer_complete_flag == false && timeout_us>0)
	{
		timeout_us--;
		R_BSP_SoftwareDelay(1U, BSP_DELAY_UNITS_MICROSECONDS);
	}

	if (spi_transfer_complete_flag == false)
	{
		/* 超时处理 */
		printf("[ST7735] CMD超时\r\n");
	}

	spi_transfer_complete_flag = false;  // 清除标志

	R_IOPORT_PinWrite(&g_ioport_ctrl, TFT_CS_Pin, BSP_IO_LEVEL_HIGH);
}

void ST7735_WriteByte(uint8_t data){
    R_IOPORT_PinWrite(&g_ioport_ctrl, TFT_DC_Pin, BSP_IO_LEVEL_HIGH);
    R_IOPORT_PinWrite(&g_ioport_ctrl, TFT_CS_Pin, BSP_IO_LEVEL_LOW);

    R_SCI_SPI_Write(&g_spi0_ctrl, &data, 1, SPI_BIT_WIDTH_8_BITS);

	/* 添加超时机制，避免无限等待 */
	timeout_us = SPI_DATA_TIMEOUT_us;

	while (spi_transfer_complete_flag == false && timeout_us>0)
	{
		timeout_us--;
		R_BSP_SoftwareDelay(1U, BSP_DELAY_UNITS_MICROSECONDS);
	}

	if (spi_transfer_complete_flag == false)
	{
		/* 超时处理 */
		printf("[ST7735] Byte超时\r\n");
	}

	spi_transfer_complete_flag = false;  // 清除标志

    R_IOPORT_PinWrite(&g_ioport_ctrl, TFT_CS_Pin, BSP_IO_LEVEL_HIGH);
}

void ST7735_WriteData(uint8_t *data, size_t data_size){
    R_IOPORT_PinWrite(&g_ioport_ctrl, TFT_DC_Pin, BSP_IO_LEVEL_HIGH);
    R_IOPORT_PinWrite(&g_ioport_ctrl, TFT_CS_Pin, BSP_IO_LEVEL_LOW);

    R_SCI_SPI_Write(&g_spi0_ctrl, data, data_size, SPI_BIT_WIDTH_8_BITS);

	/* 添加超时机制，避免无限等待 */
	timeout_us = SPI_DATA_TIMEOUT_us;

	while (spi_transfer_complete_flag == false && timeout_us>0)
	{
		timeout_us--;
		R_BSP_SoftwareDelay(1U, BSP_DELAY_UNITS_MICROSECONDS);
	}

	if (spi_transfer_complete_flag == false)
	{
		/* 超时处理 */
		printf("[ST7735] DATA超时\r\n");
	}

	spi_transfer_complete_flag = false;  // 清除标志

	R_IOPORT_PinWrite(&g_ioport_ctrl, TFT_CS_Pin, BSP_IO_LEVEL_HIGH);
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
	R_BSP_SoftwareDelay(120U, BSP_DELAY_UNITS_MILLISECONDS);

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
	ST7735_WriteByte(0xA8);
	ST7735_WriteByte(0x08);
	ST7735_WriteByte(0x84);
	ST7735_WriteCmd(ST7735_PWCTR2);
	ST7735_WriteByte(0xC4);
	ST7735_WriteCmd(ST7735_PWCTR3);
	ST7735_WriteByte(0x0D);
	ST7735_WriteByte(0x00);
	ST7735_WriteCmd(ST7735_PWCTR4);
	ST7735_WriteByte(0x8D);
	ST7735_WriteByte(0x6A);
	ST7735_WriteCmd(ST7735_PWCTR5);
	ST7735_WriteByte(0x8D);
	ST7735_WriteByte(0xEE);
	ST7735_WriteCmd(ST7735_VMCTR);
	ST7735_WriteByte(0x05);

    ST7735_WriteCmd(ST7735_GAMCTRP);
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
    ST7735_WriteCmd(ST7735_GAMCTRN);
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

	ST7735_WriteCmd(ST7735_INVOFF);
	ST7735_WriteCmd(ST7735_COLMOD);
	ST7735_WriteByte(0x05); // 16-bit color
	ST7735_WriteCmd(ST7735_CASET);
	ST7735_WriteByte(0x00);
	ST7735_WriteByte(0x00);
	ST7735_WriteByte((ST7735_WIDTH - 1) >> 8);
	ST7735_WriteByte(ST7735_WIDTH - 1);
	ST7735_WriteCmd(ST7735_RASET);
	ST7735_WriteByte(0x00);
	ST7735_WriteByte(0x00);
	ST7735_WriteByte((ST7735_HEIGHT - 1) >> 8);
	ST7735_WriteByte(ST7735_HEIGHT - 1);
	ST7735_WriteCmd(ST7735_MADCTL);
	ST7735_WriteByte(ST7735_MADCTL_Parameter);

	ST7735_WriteCmd(ST7735_DISPON);
	SCREEN_FillScreen(SCREEN_BLACK);
	SCREEN_RefreshScreen();
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
	data[0] = (uint8_t)(x0 >> 8);  // 起始列高字节
	data[1] = (uint8_t)(x0 & 0xFF); // 起始列低字节
	data[2] = (uint8_t)(x1 >> 8);  // 结束列高字节
	data[3] = (uint8_t)(x1 & 0xFF); // 结束列低字节
	ST7735_WriteData(data, sizeof(data));

	ST7735_WriteCmd(ST7735_RASET); // Row
	data[0] = (uint8_t)(y0 >> 8);  // 起始行高字节
	data[1] = (uint8_t)(y0 & 0xFF); // 起始行低字节
	data[2] = (uint8_t)(y1 >> 8);  // 结束行高字节
	data[3] = (uint8_t)(y1 & 0xFF); // 结束行低字节
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
	else if(ReadPixel(x,y) == Pixel_Set)
		return ;
	display_ram[y][x] = Pixel_Set;
#ifdef ST7735_PARTIAL_REFRESH
	// 更新列范围
	if (!dirty_row[y]) {
		min_col[y] = max_col[y] = x;
		dirty_row[y] = true;
	} else {
		if (x < min_col[y]) min_col[y] = x;
		if (x > max_col[y]) max_col[y] = x;
	}
#endif
}

/**
 * @brief 绘制水平线 
 */
void DrawHorLine(int16_t x0, int16_t x1, int16_t y, SCREEN_Pixel_t Pixel_Set, SCREEN_Mode_t type){
	// 边界裁剪
	if (y < 0 || y >= ST7735_HEIGHT) return;
	int16_t x_start = (x0 < x1) ? x0 : x1;
	int16_t x_end   = (x0 < x1) ? x1 : x0;
	if (x_start < 0) x_start = 0;
	if (x_end >= ST7735_WIDTH) x_end = ST7735_WIDTH - 1;
	if (x_start > x_end) return;

	bool changed = false;
	for (int16_t x = x_start; x <= x_end; x++) {
		SCREEN_Pixel_t val;
		if (type == SCREEN_Xor && display_ram[y][x] == Pixel_Set)
			val = ~Pixel_Set;
		else
			val = Pixel_Set;
		if (display_ram[y][x] != val) {
			display_ram[y][x] = val;
			changed = true;
		}
	}

#ifdef ST7735_PARTIAL_REFRESH
	// 批量更新 dirty tracking（O(1) 而非逐像素 O(n)）
	if (changed) {
		if (!dirty_row[y]) {
			min_col[y] = x_start;
			max_col[y] = x_end;
			dirty_row[y] = true;
		} else {
			if (x_start < min_col[y]) min_col[y] = x_start;
			if (x_end > max_col[y]) max_col[y] = x_end;
		}
	}
#endif
}

/**
 * @brief 绘制垂直线 
 */
void DrawVerLine(int16_t x, int16_t y0, int16_t y1, SCREEN_Pixel_t Pixel_Set, SCREEN_Mode_t type){
	// 边界裁剪
	if (x < 0 || x >= ST7735_WIDTH) return;
	int16_t y_start = (y0 < y1) ? y0 : y1;
	int16_t y_end   = (y0 < y1) ? y1 : y0;
	if (y_start < 0) y_start = 0;
	if (y_end >= ST7735_HEIGHT) y_end = ST7735_HEIGHT - 1;
	if (y_start > y_end) return;

	for (int16_t y = y_start; y <= y_end; y++) {
		SCREEN_Pixel_t val;
		if (type == SCREEN_Xor && display_ram[y][x] == Pixel_Set)
			val = ~Pixel_Set;
		else
			val = Pixel_Set;

		if (display_ram[y][x] == val) continue;
		display_ram[y][x] = val;

#ifdef ST7735_PARTIAL_REFRESH
		if (!dirty_row[y]) {
			min_col[y] = max_col[y] = x;
			dirty_row[y] = true;
		} else {
			if (x < min_col[y]) min_col[y] = x;
			if (x > max_col[y]) max_col[y] = x;
		}
#endif
	}
}

/**
 * @brief 填充整个屏幕
 */
void SCREEN_FillScreen(SCREEN_Pixel_t Pixel_Set) {
	// RGB565: 高低字节相同时（如0x0000黑、0xFFFF白）可用 memset 加速
	uint8_t hi = (uint8_t)(Pixel_Set >> 8);
	uint8_t lo = (uint8_t)(Pixel_Set & 0xFF);
	if (hi == lo) {
		memset(display_ram, hi, sizeof(display_ram));
	} else {
		for (uint32_t i = 0; i < ST7735_HEIGHT * ST7735_WIDTH; i++) {
			((SCREEN_Pixel_t *)display_ram)[i] = Pixel_Set;
		}
	}
#ifdef ST7735_PARTIAL_REFRESH
	// 标记所有行为脏，全列范围
	for (uint16_t y = 0; y < ST7735_HEIGHT; y++) {
		min_col[y] = 0;
		max_col[y] = ST7735_WIDTH - 1;
		dirty_row[y] = true;
	}
#endif
}

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
 * @brief 刷新脏区域标记的屏幕（混合策略：少量脏行逐行精确刷新，大量脏行走全屏刷新）
 * @note 时间测量单位：毫秒(ms)
 */
uint32_t SCREEN_RefreshScreen(void) {
	// 记录本次刷新开始时间
	uint32_t start_time = SysTime_Get();

#ifdef ST7735_PARTIAL_REFRESH
	static uint8_t buff[ST7735_WIDTH * 2];

	// 统计脏行数量，决定刷新策略
	uint16_t dirty_count = 0;
	for (uint16_t y = 0; y < ST7735_HEIGHT; y++) {
		if (dirty_row[y]) dirty_count++;
	}

	if (dirty_count == 0) {
		// 无脏区域，跳过
	} else if (dirty_count >= ST7735_HEIGHT / 2) {
		// 超过一半行脏，全屏刷新更高效（避免逐行 SetAddressWindow 开销）
		SCREEN_RefreshScreen_Force();
		for (uint16_t y = 0; y < ST7735_HEIGHT; y++) {
			dirty_row[y] = false;
		}
	} else {
		// 少量脏行：逐行精确刷新，每行仅发送实际改变的列范围
		for (uint16_t y = 0; y < ST7735_HEIGHT; y++) {
			if (!dirty_row[y]) continue;

			uint8_t x0 = min_col[y];
			uint8_t x1 = max_col[y];
			uint16_t row_width = x1 - x0 + 1;

			// 设置单行窗口
			ST7735_SetAddressWindow(x0, y, x1, y);
			ST7735_WriteCmd(ST7735_RAMWR);

			// 准备行数据
			for (uint16_t i = 0; i < row_width; i++) {
				buff[i * 2]     = (uint8_t)(display_ram[y][x0 + i] >> 8);
				buff[i * 2 + 1] = (uint8_t)(display_ram[y][x0 + i] & 0xFF);
			}
			ST7735_WriteData(buff, row_width * 2);

			dirty_row[y] = false;
		}
	}
#else
	SCREEN_RefreshScreen_Force();
#endif

	// 记录本次刷新结束时间
	uint32_t end_time = SysTime_Get();
	uint32_t elapsed_cycles = SysTime_Elapsed(start_time, end_time);
	uint32_t elapsed_ms = SysTime_CyclesToMs(elapsed_cycles);

	return elapsed_ms;
}
