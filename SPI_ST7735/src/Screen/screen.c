#include "screen.h"
#include "stdlib.h"

// 定义字体数据（只在此编译单元中定义一次）
#define FONTS_DATA_EXTERN
#include "fonts.h"

/**
 * @brief 填充页面
 * @param Pixel_Set: 填充颜色
 */
void SCREEN_FillScreen(SCREEN_Pixel_t Pixel_Set)
{
	FillScreen(Pixel_Set);
}

/**
 * @brief 绘制像素
 * @param x0: 像素水平坐标
 * @param y0: 像素垂直坐标
 * @param Pixel_Set: 绘制图像的颜色
 * @param type: 绘制模式,SCREEN_Xor 异或模式,SCREEN_Nor 普通模式
 */
SCREEN_Event_t SCREEN_DrawPixel(int16_t x0, int16_t y0,
								SCREEN_Pixel_t Pixel_Set, SCREEN_Mode_t type)
{
	if((type == SCREEN_Xor)&&(ReadPixel(x0,y0) == Pixel_Set))
		DrawPixel(x0, y0, ~Pixel_Set);
	else
		DrawPixel(x0, y0, Pixel_Set);

	return SCREEN_OK;
}

/**
 * @brief 绘制直线 - Bresenham算法
 * @param x0,x1: 直线水平坐标
 * @param y0,y1: 直线垂直坐标
 * @param Pixel_Set: 绘制图像的颜色
 * @param type: 绘制模式,SCREEN_Xor 异或模式,SCREEN_Nor 普通模式
 */
SCREEN_Event_t SCREEN_DrawLine(int16_t x0, int16_t x1, int16_t y0, int16_t y1,
								SCREEN_Pixel_t Pixel_Set, SCREEN_Mode_t type)
{
	int16_t dx = (x1 > x0) ? (x1 - x0) : (x0 - x1);
	int16_t dy = (y1 > y0) ? (y1 - y0) : (y0 - y1);
	int16_t sx = (x0 < x1) ? 1 : -1;
	int16_t sy = (y0 < y1) ? 1 : -1;
	int16_t err = dx - dy;
	int16_t e2;

	while(1) {
		SCREEN_DrawPixel(x0, y0, Pixel_Set, type);
			
		if (x0 == x1 && y0 == y1) break;
		
		e2 = 2 * err;
		if (e2 > -dy) {
			err -= dy;
			x0 += sx;
		}
		if (e2 < dx) {
			err += dx;
			y0 += sy;
		}
	}
	
	return SCREEN_OK;
}

/**
 * @brief 绘制实心矩形
 * @param x0,x1: 矩形水平坐标起始点
 * @param y0,y1: 矩形垂直坐标起始点
 * @param Pixel_Set: 绘制图像的颜色(二值)
 * @param type: 绘制模式,SCREEN_Xor 异或模式,SCREEN_Nor 普通模式
 */
SCREEN_Event_t SCREEN_DrawRectSolid(int16_t x0, int16_t x1, int16_t y0, int16_t y1,
								SCREEN_Pixel_t Pixel_Set, SCREEN_Mode_t type)
{
    int16_t x_start = (x0 < x1)? x0 : x1;
    int16_t x_end   = (x0 < x1)? x1 : x0;

    int16_t y_start = (y0 < y1)? y0 : y1;
    int16_t y_end   = (y0 < y1)? y1 : y0;
    
	for(int16_t y = y_start; y <= y_end; y++) {
		for(int16_t x = x_start; x <= x_end; x++) {
			SCREEN_DrawPixel(x, y, Pixel_Set, type);
		}
	}
    
	return SCREEN_OK;
}

/**
 * @brief 绘制空心矩形
 * @param x0,x1: 矩形水平坐标起始点
 * @param y0,y1: 矩形垂直坐标起始点
 * @param Pixel_Set: 绘制图像的颜色(二值)
 * @param type: 绘制模式,SCREEN_Xor 异或模式,SCREEN_Nor 普通模式
 */
SCREEN_Event_t SCREEN_DrawRectHollow(int16_t x0, int16_t x1, int16_t y0, int16_t y1,
								SCREEN_Pixel_t Pixel_Set, SCREEN_Mode_t type)
{
	// 使用直线函数绘制四条边
	DrawHorLine(x0, x1, y0, Pixel_Set, type); // 上边
	DrawHorLine(x0, x1, y1, Pixel_Set, type); // 下边
	DrawVerLine(x0, y0+1, y1-1, Pixel_Set, type); // 左边
	DrawVerLine(x1, y0+1, y1-1, Pixel_Set, type); // 右边

	return SCREEN_OK;
}

/**
 * @brief 绘制圆角实心矩形
 * @param x0,x1: 矩形水平坐标起始点
 * @param y0,y1: 矩形垂直坐标起始点
 * @param radius: 圆角半径
 * @param Pixel_Set: 绘制图像的颜色(二值)
 * @param type: 绘制模式
 */
SCREEN_Event_t SCREEN_DrawRoundRectSolid(int16_t x0, int16_t x1, int16_t y0, int16_t y1,
									uint8_t radius, SCREEN_Pixel_t Pixel_Set, SCREEN_Mode_t type)
{
	int16_t x_start = (x0 > x1) ? x1 : x0;
	int16_t x_end   = (x0 > x1) ? x0 : x1;

	int16_t y_start = (y0 > y1) ? y1 : y0;
	int16_t y_end   = (y0 > y1) ? y0 : y1;

	// 计算矩形尺寸
	uint16_t rect_width  = (uint16_t)(x_end - x_start);
	uint16_t rect_height = (uint16_t)(y_end - y_start);

	// 限制圆角半径
	if (radius > rect_width / 2) radius = rect_width / 2;
	if (radius > rect_height / 2) radius = rect_height / 2;

	// 如果半径为0，绘制普通实心矩形
	if (radius == 0) {
		return SCREEN_DrawRectSolid(x_start, x_end, y_start, y_end, Pixel_Set, type);
	}

	// 使用Bresenham算法计算圆角边界
	int16_t x = 0;
	int16_t y = radius;
	int16_t d = 3 - 2 * radius;

	// 存储每行的左右边界
	int16_t left_bound[radius + 1];
	int16_t right_bound[radius + 1];

	// 初始化边界数组
	for (int i = 0; i <= radius; i++) {
		left_bound[i] = radius;
		right_bound[i] = radius;
	}

	// 计算圆角边界
	while (x <= y) {
		// 记录当前行的边界
		left_bound[y] = x;
		left_bound[x] = y;

		if (d < 0) {
			d = d + 4 * x + 6;
		} else {
			d = d + 4 * (x - y) + 10;
			y--;
		}
		x++;
	}

	// 计算圆心坐标
	int16_t center_x[4] = {x_start + radius, x_end - radius, x_start + radius, x_end - radius};
	int16_t center_y[4] = {y_start + radius, y_start + radius, y_end - radius, y_end - radius};

	// 逐行绘制
	for (int16_t row = y_start; row <= y_end; row++) {
		int16_t line_start = x_start;
		int16_t line_end = x_end;

		// 处理顶部圆角
		if (row < y_start + radius) {
			int16_t dy = row - (y_start + radius);
			int16_t offset = left_bound[abs(dy)];
			line_start = x_start + (radius - offset);
			line_end = x_end - (radius - offset);
		}
		// 处理底部圆角
		else if (row > y_end - radius) {
			int16_t dy = row - (y_end - radius);
			int16_t offset = left_bound[abs(dy)];
			line_start = x_start + (radius - offset);
			line_end = x_end - (radius - offset);
		}

		// 绘制当前行
		if (line_start <= line_end) {
			DrawHorLine(line_start, line_end, row, Pixel_Set, type);
		}
	}

	return SCREEN_OK;
}

/**
 * @brief 绘制圆角空心矩形
 * @param x0,x1: 矩形水平坐标起始点
 * @param y0,y1: 矩形垂直坐标起始点
 * @param radius: 圆角半径
 * @param Pixel_Set: 绘制图像的颜色(二值)
 * @param type: 绘制模式
 */
SCREEN_Event_t SCREEN_DrawRoundRectHollow(int16_t x0, int16_t x1, int16_t y0, int16_t y1,
									uint8_t radius, SCREEN_Pixel_t Pixel_Set, SCREEN_Mode_t type)
{
	int16_t x_start = (x0 > x1) ? x1 : x0;
	int16_t x_end   = (x0 > x1) ? x0 : x1;

	int16_t y_start = (y0 > y1) ? y1 : y0;
	int16_t y_end   = (y0 > y1) ? y0 : y1;

	// 限制圆角半径不超过矩形尺寸的一半
	uint16_t rect_width = x_end - x_start;
	uint16_t rect_height = y_end - y_start;
	if (radius > rect_width / 2) radius = rect_width / 2;
	if (radius > rect_height / 2) radius = rect_height / 2;

	// 如果半径为0，绘制普通矩形
	if (radius == 0) {
		return SCREEN_DrawRectHollow(x_start, x_end, y_start, y_end, Pixel_Set, type);
	}

	// 计算圆角圆心坐标
	int16_t top_left_center_x = x_start + radius;
	int16_t top_left_center_y = y_start + radius;
	int16_t top_right_center_x = x_end - radius;
	int16_t top_right_center_y = y_start + radius;
	int16_t bottom_left_center_x = x_start + radius;
	int16_t bottom_left_center_y = y_end - radius;
	int16_t bottom_right_center_x = x_end - radius;
	int16_t bottom_right_center_y = y_end - radius;

	// 绘制四条直线边（精确控制端点，避免与圆角重叠）
	// 上边（左右圆角之间，不包含圆角部分）
	if (rect_width > 2 * radius) {
		DrawHorLine(x_start + radius + 1, x_end - radius - 1, y_start, Pixel_Set, type);
	}

	// 下边（左右圆角之间，不包含圆角部分）
	if (rect_width > 2 * radius) {
		DrawHorLine(x_start + radius + 1, x_end - radius - 1, y_end, Pixel_Set, type);
	}

	// 左边（上下圆角之间，不包含圆角部分）
	if (rect_height > 2 * radius) {
		DrawVerLine(x_start, y_start + radius + 1, y_end - radius - 1, Pixel_Set, type);
	}

	// 右边（上下圆角之间，不包含圆角部分）
	if (rect_height > 2 * radius) {
		DrawVerLine(x_end, y_start + radius + 1, y_end - radius - 1, Pixel_Set, type);
	}

	// 绘制四个圆角（完整的90度圆弧）
	// 左上圆角（第二象限）
	SCREEN_DrawQuarArc(top_left_center_x, top_left_center_y, radius, 0x02, Pixel_Set, type);
	// 右上圆角（第一象限）
	SCREEN_DrawQuarArc(top_right_center_x, top_right_center_y, radius, 0x01, Pixel_Set, type);
	// 左下圆角（第三象限）
	SCREEN_DrawQuarArc(bottom_left_center_x, bottom_left_center_y, radius, 0x04, Pixel_Set, type);
	// 右下圆角（第四象限）
	SCREEN_DrawQuarArc(bottom_right_center_x, bottom_right_center_y, radius, 0x08, Pixel_Set, type);

	return SCREEN_OK;
}

/**
 * @brief 绘制四分之一圆弧 - Bresenham算法
 * @param x0,y0: 圆弧的圆心
 * @param r: 圆弧的半径
 * @param quadrant_mask: 绘制的象限 Quarter1,Quarter2,Quarter3,Quarter4
 * @param Pixel_Set: 绘制图像的颜色(二值)
 * @param type: 绘制模式,SCREEN_Xor 异或模式,SCREEN_Nor 普通模式
 */
SCREEN_Event_t SCREEN_DrawQuarArc(int16_t x0, int16_t y0, uint16_t r, uint8_t quadrant_mask,
								SCREEN_Pixel_t Pixel_Set, SCREEN_Mode_t type)
{
	if (r == 0) {
		//SCREEN_DrawPixel(x0, y0, Pixel_Set);
		return SCREEN_OK;
	}

	// 使用静态数组缓存圆弧点（适用于多次绘制相同半径的圆弧）
	static uint16_t last_r = 0;
	static int16_t arc_points[64][2]; // 假设最大半径对应的点数
	static uint16_t point_count = 0;

	// 如果半径改变，重新计算圆弧点
	if (r != last_r) {
		point_count = 0;
		int16_t x = 0;
		int16_t y = r;
		int16_t d = 3 - 2 * r;

		while(x <= y) {
			// 存储第一组点 (x,y)
			arc_points[point_count][0] = x;
			arc_points[point_count][1] = y;
			point_count++;

			// 存储第二组点 (y,x) - 只有当 x < y 时
			if (x < y) {
				arc_points[point_count][0] = y;
				arc_points[point_count][1] = x;
				point_count++;
			}

			// Bresenham算法更新
			if(d < 0) {
				d = d + 4 * x + 6;
			} else {
				d = d + 4 * (x - y) + 10;
				y--;
			}
			x++;
		}
		last_r = r;
	}

	// 使用预计算的点绘制圆弧
	for (uint16_t i = 0; i < point_count; i++) {
		int16_t x = arc_points[i][0];
		int16_t y = arc_points[i][1];

		int16_t x_temp, y_temp;
		if (quadrant_mask & 0x01) {
			x_temp = x0 + x; y_temp = y0 - y;
			SCREEN_DrawPixel(x_temp, y_temp, Pixel_Set, type);
		}

		if (quadrant_mask & 0x02) {
			x_temp = x0 - x; y_temp = y0 - y;
			SCREEN_DrawPixel(x_temp, y_temp, Pixel_Set, type);
		}

		if (quadrant_mask & 0x04) {
			x_temp = x0 - x; y_temp = y0 + y;
			SCREEN_DrawPixel(x_temp, y_temp, Pixel_Set, type);
		}

		if (quadrant_mask & 0x08) {
			x_temp = x0 + x; y_temp = y0 + y;
			SCREEN_DrawPixel(x_temp, y_temp, Pixel_Set, type);
		}
	}

	return SCREEN_OK;
}

// 辅助函数：更新边界数组
static void updateBounds(int16_t x, int16_t y, int16_t *min_x, int16_t *max_x)
{
	if(y >= 0 && y < ST7735_HEIGHT) {
		if(x < min_x[y]) min_x[y] = x;
		if(x > max_x[y]) max_x[y] = x;
	}
}

/**
 * @brief 绘制四分之一扇形 
 * @param x0,y0: 圆弧的圆心
 * @param r: 圆弧的半径
 * @param quadrant_mask: 绘制的象限 Quarter1,Quarter2,Quarter3,Quarter4
 * @param Pixel_Set: 绘制图像的颜色(二值)
 * @param type: 绘制模式,SCREEN_Xor 异或模式,SCREEN_Nor 普通模式
 */
SCREEN_Event_t SCREEN_DrawQuarSector(int16_t x0, int16_t y0, uint16_t r, uint8_t quadrant_mask,
								SCREEN_Pixel_t Pixel_Set, SCREEN_Mode_t type)
{
    if (r == 0) {
        return SCREEN_PARAM_ERROR;
    }
    
    // 预计算圆弧的所有点
    int16_t x = 0;
    int16_t y = r;
    int16_t d = 3 - 2 * r;
    
    // 存储每行的最小和最大x值
    int16_t min_x[ST7735_HEIGHT] = {0};
    int16_t max_x[ST7735_HEIGHT] = {0};
    
    // 初始化数组
    for(int16_t i = 0; i < ST7735_HEIGHT; i++) {
        min_x[i] = ST7735_WIDTH;
        max_x[i] = -1;
    }
    
    // 扫描圆弧边界
    while(x <= y) {
        // 更新四个象限的边界
        if(quadrant_mask & 0x01) { // 右上
            updateBounds(x0 + x, y0 - y, min_x, max_x);
            if(x < y) updateBounds(x0 + y, y0 - x, min_x, max_x);
        }
        if(quadrant_mask & 0x02) { // 左上
            updateBounds(x0 - x, y0 - y, min_x, max_x);
            if(x < y) updateBounds(x0 - y, y0 - x, min_x, max_x);
        }
        if(quadrant_mask & 0x04) { // 左下
            updateBounds(x0 - x, y0 + y, min_x, max_x);
            if(x < y) updateBounds(x0 - y, y0 + x, min_x, max_x);
        }
        if(quadrant_mask & 0x08) { // 右下
            updateBounds(x0 + x, y0 + y, min_x, max_x);
            if(x < y) updateBounds(x0 + y, y0 + x, min_x, max_x);
        }
        
        // Bresenham算法更新
        if(d < 0) {
            d = d + 4 * x + 6;
        } else {
            d = d + 4 * (x - y) + 10;
            y--;
        }
        x++;
    }
    
    // 填充扇形区域
    for(int16_t row = 0; row < ST7735_HEIGHT; row++) {
        if(min_x[row] <= max_x[row]) {
            DrawHorLine(min_x[row], max_x[row], row, Pixel_Set, type);
        }
    }
    
    return SCREEN_OK;
}

/**
 * @brief 绘制ASCII字符
 * @param x0,y0 绘制字符的左上角
 * @param ch 要显示的字符
 * @param font 字体结构体指针
 * @param font_t 字体类型
 * @param Pixel_Set: 绘制图像的颜色(二值)
 * @param type: 绘制模式,SCREEN_Xor 异或模式,SCREEN_Nor 普通模式
 */
SCREEN_Event_t SCREEN_DrawChar(int16_t x0, int16_t y0, char ch, const struFont_t *font,
									SCREEN_Pixel_t Pixel_Set, SCREEN_Mode_t type)
{	
    // 参数检查
    if (font == NULL || ch < ' ' || ch > '~') {
		return SCREEN_CHAR_EXCEED;
    }

    // 获取字符索引（从空格开始）
    uint8_t char_index = ch - ' ';
    const uint8_t *char_data = font->font_data;
    
    if (char_data == NULL) {
        return SCREEN_CHAR_EXCEED;
    }
    
    // 定位字符数据
    char_data += char_index * font->height * font->bytes_per_row;

    // 逐像素绘制
    for(uint8_t row = 0; row < font->height; row++) {
        for(uint8_t col = 0; col < font->width; col++) {
            // 计算字节和位索引
            uint8_t byte_index = col / 8;
            uint8_t bit_index = col % 8;
            uint8_t current_byte = char_data[row * font->bytes_per_row + byte_index];

            if(current_byte & (0x01 << bit_index)) {
                SCREEN_DrawPixel(x0 + col, y0 + row, Pixel_Set, type);
            }
        }
    }
    
    return SCREEN_OK;
}

/**
 * @brief 绘制ASCII字符串
 * @note  str: 绘制的字符串
 */
SCREEN_Event_t SCREEN_DrawString(int16_t x0, int16_t y0, const char *str, const struFont_t *font,
									SCREEN_Pixel_t Pixel_Set, SCREEN_Mode_t type)
{
    // 修正坐标系
	uint16_t current_x = x0;
	uint16_t current_y = y0;
	
	while(*str != '\0')
	{
		SCREEN_DrawChar(current_x, current_y, *str, font, Pixel_Set, type);
		
		// 水平前进
		current_x += font->width;
	
		// 检查换行
		if(current_x + font->width >= ST7735_WIDTH)
		{
			return SCREEN_OK;
//			current_x = x0; // 回到起始 x
//			current_y += font->height; // 换到下一行 (y 增加)
//	
//			if(current_y + font->height >= ST7735_HEIGHT)
//			{
//				break; // 超出屏幕底部
//			}
		}
		
		str++; 
	}
	return SCREEN_OK;
}

/**
 * @brief 绘制UTF-8汉字（单个）
 * @param x0,y0 绘制字符的左上角
 * @param utf8_char UTF-8编码的汉字字符串指针（3字节）
 * @param hz_font 汉字字体结构体指针
 * @param Pixel_Set: 绘制图像的颜色(二值)
 * @param type: 绘制模式,SCREEN_Xor 异或模式,SCREEN_Nor 普通模式
 * @note 字模格式：低位在前，逐行，阴码
 */
SCREEN_Event_t SCREEN_DrawUTFChar(int16_t x0, int16_t y0, const char *utf8_char, const struFont_UTF_t *hz_font,
									SCREEN_Pixel_t Pixel_Set, SCREEN_Mode_t type)
{
	if (utf8_char == NULL || hz_font == NULL) {
		return SCREEN_PARAM_ERROR;
	}

	// 在汉字表中查找UTF-8编码
	const struFont_UTF_data_t *table = hz_font->font_UTF_data;
	const uint8_t *target = (const uint8_t *)utf8_char;

	// 线性搜索查找汉字（可优化为二分查找）
	int i = 0;
	while (1) {
		// 检查是否到达表尾（UTF8_code全0表示结束）
		if (table[i].UTF8_code[0] == 0 && table[i].UTF8_code[1] == 0) {
			break;  // 未找到
		}

		// 比较UTF-8编码（比较前2字节）
		if (table[i].UTF8_code[0] == target[0] && table[i].UTF8_code[1] == target[1]) {
			// 找到汉字，开始绘制
			const uint8_t *char_data = table[i].font_data;  // 获取字模数据

			// 逐行、逐像素绘制（低位在前，逐行，阴码）
			for (uint8_t row = 0; row < hz_font->height; row++) {
				for (uint8_t col = 0; col < hz_font->width; col++) {
					// 计算字节索引和位索引
					uint8_t byte_index = col / 8;
					uint8_t bit_index = col % 8;  // 低位在前：bit0是最左像素
					uint8_t current_byte = char_data[row * hz_font->bytes_per_row + byte_index];

					// 阴码：1表示点亮像素
					if (current_byte & (0x01 << bit_index)) {
						SCREEN_DrawPixel(x0 + col, y0 + row, Pixel_Set, type);
					}
				}
			}
			return SCREEN_OK;
		}
		i++;
	}

	return SCREEN_CHAR_EXCEED;  // 未找到该汉字
}

/**
 * @brief 获取UTF-8字符的字节长度
 * @param c UTF-8字符串的首字节
 * @return 字符占用字节数(1-3)
 */
static uint8_t UTF8_GetCharLen(uint8_t c)
{
	if ((c & 0x80) == 0x00) {
		return 1;  // ASCII: 0xxxxxxx
	} else if ((c & 0xE0) == 0xC0) {
		return 2;  // 2字节: 110xxxxx
	} else if ((c & 0xF0) == 0xE0) {
		return 3;  // 3字节(汉字): 1110xxxx
	} else if ((c & 0xF8) == 0xF0) {
		return 4;  // 4字节: 11110xxx
	}
	return 1;  // 默认按1字节处理
}

/**
 * @brief 绘制UTF-8字符串(支持ASCII和汉字混合)
 * @param x0,y0 绘制字符串的左上角
 * @param utf8_str UTF-8编码的字符串
 * @param ascii_font ASCII字体结构体指针
 * @param hz_font 汉字字体结构体指针
 * @param Pixel_Set: 绘制图像的颜色(二值)
 * @param type: 绘制模式,SCREEN_Xor 异或模式,SCREEN_Nor 普通模式
 */
SCREEN_Event_t SCREEN_DrawUTF8String(int16_t x0, int16_t y0, const char *utf8_str,
									const struFont_t *ascii_font, const struFont_UTF_t *hz_font,
									SCREEN_Pixel_t Pixel_Set, SCREEN_Mode_t type)
{
	if (utf8_str == NULL || ascii_font == NULL || hz_font == NULL) {
		return SCREEN_PARAM_ERROR;
	}

	uint16_t current_x = x0;
	uint16_t current_y = y0;
	const char *p = utf8_str;

	while (*p != '\0') {
		uint8_t char_len = UTF8_GetCharLen((uint8_t)*p);

		// 检查X边界
		uint16_t char_width = (char_len == 1) ? ascii_font->width : hz_font->width;
		if (current_x + char_width > ST7735_WIDTH) {
			break;  // 超出屏幕宽度
		}

		if (char_len == 1) {
			// ASCII字符
			if (*p >= ' ' && *p <= '~') {
				SCREEN_DrawChar(current_x, current_y, *p, ascii_font, Pixel_Set, type);
			}
			current_x += ascii_font->width;
			p++;
		} else if (char_len == 3) {
			// UTF-8汉字(3字节)
			SCREEN_Event_t ret = SCREEN_DrawUTFChar(current_x, current_y, p, hz_font, Pixel_Set, type);
			if (ret == SCREEN_OK) {
				current_x += hz_font->width;
			} else {
				// 未找到汉字，绘制占位符或跳过
				current_x += hz_font->width;
			}
			p += 3;
		} else {
			// 其他UTF-8字符(2字节或4字节)，暂时跳过
			p += char_len;
		}

//		// 可选：自动换行
//		if (current_x + char_width >= ST7735_WIDTH) {
//			current_x = x0;
//			current_y += (char_len == 1) ? ascii_font->height : hz_font->height;
//			if (current_y + ((char_len == 1) ? ascii_font->height : hz_font->height) >= ST7735_HEIGHT) {
//				break;  // 超出屏幕高度
//			}
//		}
	}

	return SCREEN_OK;
}

/**
 * @brief 绘制单色位图图像(使用SCREEN_DrawPixel)
 * @param x0: 图像左上角X坐标
 * @param y0: 图像左上角Y坐标
 * @param width: 图像宽度(像素)
 * @param height: 图像高度(像素)
 * @param image: 图像数据指针
 * @param type: 绘制模式
 */
SCREEN_Event_t SCREEN_DrawImage(int16_t x0, int16_t y0, uint16_t width, uint16_t height,
								const uint8_t *image, SCREEN_Pixel_t Pixel_Set, SCREEN_Mode_t type) 
{
    // 计算每行字节数(宽度向上取整到8的倍数)
    uint16_t bytes_per_line = (width + 7) / 8;
    
    for (uint16_t row = 0; row < height; row++) {
        // 检查行是否在屏幕范围内
        int16_t current_y = y0 + row;
        if (current_y < 0 || current_y >= ST7735_HEIGHT) continue;
        
        for (uint16_t col = 0; col < width; col++) {
            // 检查列是否在屏幕范围内
            int16_t current_x = x0 + col;
            if (current_x < 0 || current_x >= ST7735_WIDTH) continue;

            // 计算字节和位位置
            uint16_t byte_index = row * bytes_per_line + (col / 8);
            uint8_t bit_index = 7 - (col % 8); // 通常位图数据是MSB在前

            uint8_t current_byte = image[byte_index];

            if(current_byte & (0x01 << bit_index)) {
                SCREEN_DrawPixel(current_x, current_y, Pixel_Set, type);
            }
        }
    }
    return SCREEN_OK;
}
