#include "lcd_init.h"
#include "delay.h"
#include "stdio.h"

/**
 * @brief       设置光标位置
 * @param       x:坐标列地址
 * @param       y:坐标行地址
 * @retval      无
 */
void LCD_SetCursor(uint16_t x, uint16_t y)
{
    LCD_Address_Set(x, y, x, y);
}

/**
 * @brief       设置显示窗口
 * @param       xs:窗口列起始地址
 * @param       ys:坐标行起始地址
 * @param       xe:窗口列结束地址
 * @param       ye:坐标行结束地址
 * @retval      无
 */
void LCD_Address_Set(uint16_t xs, uint16_t ys, uint16_t xe, uint16_t ye)
{
    if (USE_HORIZONTAL == 0)
    {
        LCD_WR_REG(0x2a); // 列地址设置
        LCD_WR_DATA(xs + 2);
        LCD_WR_DATA(xe + 2);
        LCD_WR_REG(0x2b); // 行地址设置
        LCD_WR_DATA(ys + 3);
        LCD_WR_DATA(ye + 3);
        LCD_WR_REG(0x2c); // 储存器写
    }
    else if (USE_HORIZONTAL == 1)
    {
        LCD_WR_REG(0x2a); // 列地址设置
        LCD_WR_DATA(xs + 2);
        LCD_WR_DATA(xe + 2);
        LCD_WR_REG(0x2b); // 行地址设置
        LCD_WR_DATA(ys + 1);
        LCD_WR_DATA(ye + 1);
        LCD_WR_REG(0x2c); // 储存器写
    }
    else if (USE_HORIZONTAL == 2)
    {
        LCD_WR_REG(0x2a); // 列地址设置
        LCD_WR_DATA(xs + 1);
        LCD_WR_DATA(xe + 1);
        LCD_WR_REG(0x2b); // 行地址设置
        LCD_WR_DATA(ys + 2);
        LCD_WR_DATA(ye + 2);
        LCD_WR_REG(0x2c); // 储存器写
    }
    else
    {
        LCD_WR_REG(0x2a); // 列地址设置
        LCD_WR_DATA(xs + 3);
        LCD_WR_DATA(xe + 3);
        LCD_WR_REG(0x2b); // 行地址设置
        LCD_WR_DATA(ys + 2);
        LCD_WR_DATA(ye + 2);
        LCD_WR_REG(0x2c); // 储存器写
    }
}

/**
 * @brief       指定颜色填充区域
 * @param       xs:填充区域列起始地址
 * @param       ys:填充区域行起始地址
 * @param       xe:填充区域列结束地址
 * @param       ye:填充区域行结束地址
 * @param       color:填充颜色值
 * @retval      无
 */
void LCD_Fill(uint16_t xs, uint16_t ys, uint16_t xe, uint16_t ye, uint16_t color)
{
    uint16_t i, j;
    LCD_Address_Set(xs, ys, xe - 1, ye - 1);
    for (j = ys; j < ye; j++)
    {
        for (i = xs; i < xe; i++)
        {
            LCD_WR_DATA(color);
        }
    }
}

/**
 * @brief       初始化LCD
 * @param       无
 * @retval      无
 */
void LCD_Init(void)
{
    LCD_GPIOInit();
    LCD_RES_Set();
    delay_ms(20);
    LCD_RES_Clr();
    delay_ms(20);
    LCD_RES_Set();
    delay_ms(120);
    LCD_BLK_Set();
    LCD_WR_REG(0x11); // Sleep out
    delay_ms(120);       // Delay 120ms

    LCD_WR_REG(0xB1);
    LCD_WR_DATA8(0x05);
    LCD_WR_DATA8(0x3A);
    LCD_WR_DATA8(0x3A);

    LCD_WR_REG(0xB2);
    LCD_WR_DATA8(0x05);
    LCD_WR_DATA8(0x3A);
    LCD_WR_DATA8(0x3A);

    LCD_WR_REG(0xB3);
    LCD_WR_DATA8(0x05);
    LCD_WR_DATA8(0x3A);
    LCD_WR_DATA8(0x3A);
    LCD_WR_DATA8(0x05);
    LCD_WR_DATA8(0x3A);
    LCD_WR_DATA8(0x3A);

    LCD_WR_REG(0xB4); // Dot inversion
    LCD_WR_DATA8(0x03);
    LCD_WR_DATA8(0x02);

    LCD_WR_REG(0xC0);
    LCD_WR_DATA8(0xA9);
    LCD_WR_DATA8(0x09);
    LCD_WR_DATA8(0x84);

    LCD_WR_REG(0xC1);
    LCD_WR_DATA8(0xC4);

    LCD_WR_REG(0xC2);
    LCD_WR_DATA8(0x0D);
    LCD_WR_DATA8(0x00);

    LCD_WR_REG(0xC3);
    LCD_WR_DATA8(0x8D);
    LCD_WR_DATA8(0x6A);

    LCD_WR_REG(0xC4);
    LCD_WR_DATA8(0x8D);
    LCD_WR_DATA8(0xEE);

    LCD_WR_REG(0xC5); // VCOM
    LCD_WR_DATA8(0x05);

    LCD_WR_REG(0x36); // MX, MY, RGB mode
    if (USE_HORIZONTAL == 0)
        LCD_WR_DATA8(0xC8);
    else if (USE_HORIZONTAL == 1)
        LCD_WR_DATA8(0x08);
    else if (USE_HORIZONTAL == 2)
        LCD_WR_DATA8(0x78);
    else
        LCD_WR_DATA8(0xA8);
    LCD_WR_REG(0x3A);
    LCD_WR_DATA8(0x05);

    LCD_WR_REG(0xE0);
    LCD_WR_DATA8(0x07);
    LCD_WR_DATA8(0x1E);
    LCD_WR_DATA8(0x0F);
    LCD_WR_DATA8(0x19);
    LCD_WR_DATA8(0x3A);
    LCD_WR_DATA8(0x34);
    LCD_WR_DATA8(0x2C);
    LCD_WR_DATA8(0x2E);
    LCD_WR_DATA8(0x2C);
    LCD_WR_DATA8(0x2A);
    LCD_WR_DATA8(0x31);
    LCD_WR_DATA8(0x3A);
    LCD_WR_DATA8(0x00);
    LCD_WR_DATA8(0x02);
    LCD_WR_DATA8(0x01);
    LCD_WR_DATA8(0x10);

    LCD_WR_REG(0xE1);
    LCD_WR_DATA8(0x04);
    LCD_WR_DATA8(0x1B);
    LCD_WR_DATA8(0x0B);
    LCD_WR_DATA8(0x13);
    LCD_WR_DATA8(0x30);
    LCD_WR_DATA8(0x2A);
    LCD_WR_DATA8(0x25);
    LCD_WR_DATA8(0x2A);
    LCD_WR_DATA8(0x29);
    LCD_WR_DATA8(0x27);
    LCD_WR_DATA8(0x30);
    LCD_WR_DATA8(0x3D);
    LCD_WR_DATA8(0x00);
    LCD_WR_DATA8(0x01);
    LCD_WR_DATA8(0x04);
    LCD_WR_DATA8(0x10);

    LCD_WR_REG(0x29);
}
