#include "st7735.h"
#include "sys_time/sys_time.h"

/* SPI 发送相关变量 */ 
volatile bool spi_transfer_complete_flag = false;

/* SPI 发送函数 */
bool SCREEN_SPI_Transfer(void const * p_src, uint32_t length, uint32_t timeout_us){
	spi_transfer_complete_flag = false;

	fsp_err_t err = R_SCI_SPI_Write(&g_spi0_ctrl, p_src, length, SPI_BIT_WIDTH_8_BITS);
	if (err != FSP_SUCCESS) {
		printf("[ST7735] SPI Write失败: %d\r\n", (int)err);
		return false;
	}

	/* 先紧密自旋，快速捕获短传输完成 */
	uint32_t spins = 0;
	while (!spi_transfer_complete_flag) {
		if (++spins > 100) {
			if (timeout_us == 0) break;
			timeout_us--;
			R_BSP_SoftwareDelay(1U, BSP_DELAY_UNITS_MICROSECONDS);
		}
	}

	if (!spi_transfer_complete_flag) {
		printf("[ST7735] SPI传输超时\r\n");
		return false;
	}
	return true;
}

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

/* 帧缓冲区：128x128 像素, 16位色 (RGB565) */ 
static uint16_t display_ram[ST7735_HEIGHT][ST7735_WIDTH] = {0x0000};

void SCREEN_RefreshScreen_Force(void) ;

#ifdef ST7735_PARTIAL_REFRESH
	/* 行级脏标记 */
	static bool dirty_row[ST7735_HEIGHT] = {false};
	/* 列范围脏标记：每行记录脏列的 [min, max] 范围 */
	static uint16_t dirty_col_min[ST7735_HEIGHT];
	static uint16_t dirty_col_max[ST7735_HEIGHT];
	/* 发送缓冲区：最大单行 128*2 字节 */
	static uint8_t batch_buf[ST7735_WIDTH * 2];

	/* 标记单列为脏 */
	static inline void mark_dirty(int16_t y, int16_t x) {
		if (!dirty_row[y]) {
			dirty_col_min[y] = x;
			dirty_col_max[y] = x;
			dirty_row[y] = true;
		} else {
			if (x < dirty_col_min[y]) dirty_col_min[y] = x;
			if (x > dirty_col_max[y]) dirty_col_max[y] = x;
		}
	}

	/* 标记列范围为脏 [x0, x1] */
	static inline void mark_dirty_range(int16_t y, int16_t x0, int16_t x1) {
		if (!dirty_row[y]) {
			dirty_col_min[y] = x0;
			dirty_col_max[y] = x1;
			dirty_row[y] = true;
		} else {
			if (x0 < dirty_col_min[y]) dirty_col_min[y] = x0;
			if (x1 > dirty_col_max[y]) dirty_col_max[y] = x1;
		}
	}
#endif

void SCREEN_RefreshScreen_Force(void) ;

void ST7735_Reset(void){
    R_IOPORT_PinWrite(&g_ioport_ctrl, TFT_RES_Pin, BSP_IO_LEVEL_LOW); // RES
    R_BSP_SoftwareDelay(100U, BSP_DELAY_UNITS_MILLISECONDS);
	R_IOPORT_PinWrite(&g_ioport_ctrl, TFT_RES_Pin, BSP_IO_LEVEL_HIGH); //RES
	R_BSP_SoftwareDelay(100U, BSP_DELAY_UNITS_MILLISECONDS);
}

void ST7735_WriteCmd(uint8_t cmd){
    R_IOPORT_PinWrite(&g_ioport_ctrl, TFT_DC_Pin, BSP_IO_LEVEL_LOW);
    R_IOPORT_PinWrite(&g_ioport_ctrl, TFT_CS_Pin, BSP_IO_LEVEL_LOW);

	SCREEN_SPI_Transfer(&cmd, 1, 100000);

	R_IOPORT_PinWrite(&g_ioport_ctrl, TFT_CS_Pin, BSP_IO_LEVEL_HIGH);
}

void ST7735_WriteByte(uint8_t data){
    R_IOPORT_PinWrite(&g_ioport_ctrl, TFT_DC_Pin, BSP_IO_LEVEL_HIGH);
    R_IOPORT_PinWrite(&g_ioport_ctrl, TFT_CS_Pin, BSP_IO_LEVEL_LOW);

	SCREEN_SPI_Transfer(&data, 1, 100000);

    R_IOPORT_PinWrite(&g_ioport_ctrl, TFT_CS_Pin, BSP_IO_LEVEL_HIGH);
}

void ST7735_WriteData(uint8_t *data, size_t data_size){
    R_IOPORT_PinWrite(&g_ioport_ctrl, TFT_DC_Pin, BSP_IO_LEVEL_HIGH);
    R_IOPORT_PinWrite(&g_ioport_ctrl, TFT_CS_Pin, BSP_IO_LEVEL_LOW);

	SCREEN_SPI_Transfer(data, data_size, 100000);

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
	SCREEN_RefreshScreen_Force();
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
	mark_dirty(y, x);
#endif
}

/**
 * @brief 绘制水平线 
 */
void DrawHorLine(int16_t x0, int16_t x1, int16_t y, SCREEN_Pixel_t Pixel_Set, SCREEN_Mode_t type){
	/* 边界裁剪 */
	if (y < 0 || y >= ST7735_HEIGHT) return;
	int16_t x_start = (x0 < x1) ? x0 : x1;
	int16_t x_end   = (x0 < x1) ? x1 : x0;
	if (x_start < 0) x_start = 0;
	if (x_end >= ST7735_WIDTH) x_end = ST7735_WIDTH - 1;
	if (x_start > x_end) return;

#ifdef ST7735_PARTIAL_REFRESH
	int16_t dirty_min = ST7735_WIDTH;
	int16_t dirty_max = -1;
#endif

	for (int16_t x = x_start; x <= x_end; x++) {
		SCREEN_Pixel_t val;
		if (type == SCREEN_Xor && display_ram[y][x] == Pixel_Set)
			val = ~Pixel_Set;
		else
			val = Pixel_Set;
		if (display_ram[y][x] == val) continue;
		display_ram[y][x] = val;
#ifdef ST7735_PARTIAL_REFRESH
		if (x < dirty_min) dirty_min = x;
		dirty_max = x;
#endif
	}

#ifdef ST7735_PARTIAL_REFRESH
	if (dirty_max >= dirty_min) {
		mark_dirty_range(y, dirty_min, dirty_max);
	}
#endif
}

/**
 * @brief 绘制垂直线
 */
void DrawVerLine(int16_t x, int16_t y0, int16_t y1, SCREEN_Pixel_t Pixel_Set, SCREEN_Mode_t type){
	/* 边界裁剪 */
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
		mark_dirty(y, x);
#endif
	}
}

/**
 * @brief 填充整个屏幕
 */
void SCREEN_FillScreen(SCREEN_Pixel_t Pixel_Set) {
#ifdef ST7735_PARTIAL_REFRESH
	for (int16_t y = 0; y < ST7735_HEIGHT; y++) {
		int16_t c0 = -1;
		/* 从前向后找第一个不同的像素 */
		for (int16_t x = 0; x < ST7735_WIDTH; x++) {
			if (display_ram[y][x] != Pixel_Set) { c0 = x; break; }
		}
		if (c0 < 0) continue; /* 该行无变化，跳过 */

		/* 从后向前找最后一个不同的像素 */
		int16_t c1 = c0;
		for (int16_t x = ST7735_WIDTH - 1; x > c0; x--) {
			if (display_ram[y][x] != Pixel_Set) { c1 = x; break; }
		}

		/* 只填充变化的范围 */
		for (int16_t x = c0; x <= c1; x++)
			display_ram[y][x] = Pixel_Set;
		mark_dirty_range(y, c0, c1);
	}
#else
	/* RGB565: 高低字节相同时（如0x0000黑、0xFFFF白）可用 memset 加速 */
	uint8_t hi = (uint8_t)(Pixel_Set >> 8);
	uint8_t lo = (uint8_t)(Pixel_Set & 0xFF);
	if (hi == lo) {
		memset(display_ram, hi, sizeof(display_ram));
	} else {
		for (uint32_t i = 0; i < ST7735_HEIGHT * ST7735_WIDTH; i++) {
			((SCREEN_Pixel_t *)display_ram)[i] = Pixel_Set;
		}
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
        /* 准备当前行数据 */
        for (uint16_t w = 0; w < ST7735_WIDTH; w++) {
            buff[w * 2] = (uint8_t)(display_ram[h][w] >> 8);
            buff[w * 2 + 1] = (uint8_t)(display_ram[h][w] & 0xFF);
        }
        
        /* 发送当前行数据 */
        ST7735_WriteData(buff, ST7735_WIDTH * 2);
    }
}

/**
 * @brief 刷新脏区域标记的屏幕（列范围级：每行只刷新脏列范围）
 * @note 时间测量单位：毫秒(ms)
 */
uint32_t SCREEN_RefreshScreen(void) {
	/* 记录本次刷新开始时间 */
	uint32_t start_time = SysTime_Get();

#ifdef ST7735_PARTIAL_REFRESH
	/* 逐行扫描脏行，按列范围精确刷新 */
	for (int16_t row = 0; row < ST7735_HEIGHT; row++) {
		if (!dirty_row[row]) continue;

		uint8_t c0 = dirty_col_min[row];
		uint8_t c1 = dirty_col_max[row];

		/* 设置地址窗口为该行的脏列范围 */
		ST7735_SetAddressWindow(c0, row, c1, row);
		ST7735_WriteCmd(ST7735_RAMWR);

		/* 准备并发送像素数据 */
		int buf_idx = 0;
		for (uint16_t w = c0; w <= c1; w++) {
			batch_buf[buf_idx++] = (uint8_t)(display_ram[row][w] >> 8);
			batch_buf[buf_idx++] = (uint8_t)(display_ram[row][w] & 0xFF);
		}
		ST7735_WriteData(batch_buf, buf_idx);

		dirty_row[row] = false;
	}
#else
	SCREEN_RefreshScreen_Force();
#endif

	/* 记录本次刷新结束时间 */
	uint32_t end_time = SysTime_Get();
	uint32_t elapsed_cycles = SysTime_Elapsed(start_time, end_time);
	uint32_t elapsed_ms = SysTime_CyclesToMs(elapsed_cycles);

	return elapsed_ms;
}
