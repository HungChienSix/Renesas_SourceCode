#include "spi.h"

/**
 * @brief       端口初始化配置
 * @param       无
 * @retval      无
 */
void LCD_GPIOInit(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    RCC_APB2PeriphClockCmd(LCD_SCK_GPIO_CLK|LCD_MOSI_GPIO_CLK|LCD_RES_GPIO_CLK|LCD_DC_GPIO_CLK|LCD_CS_GPIO_CLK,ENABLE);

    GPIO_InitStructure.GPIO_Pin=LCD_SCK_GPIO_PIN;
    GPIO_InitStructure.GPIO_Mode =GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed=GPIO_Speed_50MHz;
    GPIO_Init(LCD_SCK_GPIO_PORT,&GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin=LCD_MOSI_GPIO_PIN;
    GPIO_InitStructure.GPIO_Mode =GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed=GPIO_Speed_50MHz;
    GPIO_Init(LCD_MOSI_GPIO_PORT,&GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin=LCD_RES_GPIO_PIN;
    GPIO_InitStructure.GPIO_Mode =GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed=GPIO_Speed_50MHz;
    GPIO_Init(LCD_RES_GPIO_PORT,&GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin=LCD_DC_GPIO_PIN;
    GPIO_InitStructure.GPIO_Mode =GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed=GPIO_Speed_50MHz;
    GPIO_Init(LCD_DC_GPIO_PORT,&GPIO_InitStructure);
    
    GPIO_InitStructure.GPIO_Pin=LCD_CS_GPIO_PIN;
    GPIO_InitStructure.GPIO_Mode =GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed=GPIO_Speed_50MHz;
    GPIO_Init(LCD_CS_GPIO_PORT,&GPIO_InitStructure);
    
    GPIO_InitStructure.GPIO_Pin=LCD_BLK_GPIO_PIN;
    GPIO_InitStructure.GPIO_Mode =GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed=GPIO_Speed_50MHz;
    GPIO_Init(LCD_BLK_GPIO_PORT,&GPIO_InitStructure);
}

/**
 * @brief       IO模拟SPI发送一个字节数据
 * @param       dat: 需要发送的字节数据
 * @retval      无
 */
void LCD_WR_Bus(uint8_t dat)
{
    uint8_t i;
    LCD_CS_Clr();
    for (i = 0; i < 8; i++)
    {
        LCD_SCK_Clr();
        if (dat & 0x80)
        {
            LCD_MOSI_Set();
        }
        else
        {
            LCD_MOSI_Clr();
        }
        LCD_SCK_Set();
        dat <<= 1;
    }
    LCD_CS_Set();
}

/**
 * @brief       向液晶写寄存器命令
 * @param       reg: 要写的命令
 * @retval      无
 */
void LCD_WR_REG(uint8_t reg)
{
    LCD_DC_Clr();
    LCD_WR_Bus(reg);
    LCD_DC_Set();
}

/**
 * @brief       向液晶写一个字节数据
 * @param       dat: 要写的数据
 * @retval      无
 */
void LCD_WR_DATA8(uint8_t dat)
{
    LCD_DC_Set();
    LCD_WR_Bus(dat);
    LCD_DC_Set();
}

/**
 * @brief       向液晶写一个半字数据
 * @param       dat: 要写的数据
 * @retval      无
 */
void LCD_WR_DATA(uint16_t dat)
{
    LCD_DC_Set();
    LCD_WR_Bus(dat >> 8);
    LCD_WR_Bus(dat & 0xFF);
    LCD_DC_Set();
}
