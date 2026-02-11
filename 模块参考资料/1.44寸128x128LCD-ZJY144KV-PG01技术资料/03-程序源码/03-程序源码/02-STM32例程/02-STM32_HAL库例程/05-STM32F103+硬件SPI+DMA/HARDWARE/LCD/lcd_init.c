#include "lcd_init.h"
#include "dma.h"
#include "stdio.h"

extern SPI_HandleTypeDef SPI_InitStructure;
extern DMA_HandleTypeDef DMA_InitStructure;

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
    uint16_t color1[1], t = 1;
    uint32_t num, num1;
    color1[0] = color;
    num = (xe - xs) * (ye - ys) + 1;
    LCD_Address_Set(xs, ys, xe - 1, ye - 1); // 设置显示范围
    LCD_CS_Clr();
    SPI1->CR1 |= 1 << 11; // 设置SPI16位传输模式
    __HAL_SPI_ENABLE(&SPI_InitStructure);
    while (t)
    {
        if (num > 65534)
        {
            num -= 65534;
            num1 = 65534;
        }
        else
        {
            t = 0;
            num1 = num;
        }
        MYDMA_Config1(DMA1_Channel3);
        MYDMA_Enable((uint32_t)color1, (uint32_t)&SPI1->DR, num1);
        while (1)
        {
            if (__HAL_DMA_GET_FLAG(&DMA_InitStructure, DMA_FLAG_TC3)) // 等待通道4传输完成
            {
                __HAL_DMA_CLEAR_FLAG(&DMA_InitStructure, DMA_FLAG_TC3); // 清除通道3传输完成标志
                HAL_SPI_DMAStop(&SPI_InitStructure);
                break;
            }
        }
    }
    LCD_CS_Set();
    SPI1->CR1 = ~SPI1->CR1;
    SPI1->CR1 |= 1 << 11;
    SPI1->CR1 = ~SPI1->CR1; // 设置SPI8位传输模式
    __HAL_SPI_ENABLE(&SPI_InitStructure);
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
    HAL_Delay(20);
    LCD_RES_Clr();
    HAL_Delay(20);
    LCD_RES_Set();
    HAL_Delay(120);
    LCD_BLK_Set();
    LCD_WR_REG(0x11); // Sleep out
    HAL_Delay(120);       // Delay 120ms

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
    LCD_WR_REG(0x11); // Sleep out
    HAL_Delay(120);   // Delay 120ms
    LCD_WR_REG(0x29);
}
