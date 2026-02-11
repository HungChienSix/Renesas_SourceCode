#include "spi.h"

SPI_HandleTypeDef SPI_InitStructure;


/**
 * @brief       SPI1初始化
 * @param       无
 * @retval      无
 */
void SPI1_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    __HAL_RCC_SPI1_CLK_ENABLE();  // 开启SPI1外设时钟
    __HAL_RCC_GPIOA_CLK_ENABLE(); // 开启GPIO口时钟

    GPIO_InitStructure.Pin = LCD_SCK_GPIO_PIN;
    GPIO_InitStructure.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(LCD_SCK_GPIO_PORT, &GPIO_InitStructure);
    HAL_GPIO_WritePin(LCD_SCK_GPIO_PORT, LCD_SCK_GPIO_PIN, GPIO_PIN_SET);

    GPIO_InitStructure.Pin = LCD_MOSI_GPIO_PIN;
    GPIO_InitStructure.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(LCD_MOSI_GPIO_PORT, &GPIO_InitStructure);
    HAL_GPIO_WritePin(LCD_MOSI_GPIO_PORT, LCD_MOSI_GPIO_PIN, GPIO_PIN_SET);

    SPI_InitStructure.Instance = SPI1;
    SPI_InitStructure.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2; // SPI1挂载在APB2总线上面 频率72Mhz 2分频为36Mhz
    SPI_InitStructure.Init.CLKPhase = SPI_PHASE_2EDGE;                  // SPI在第二个时钟沿进行采样
    SPI_InitStructure.Init.CLKPolarity = SPI_POLARITY_HIGH;             // SPI空闲为高
    SPI_InitStructure.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE; // 关闭CRC校验
    SPI_InitStructure.Init.DataSize = SPI_DATASIZE_8BIT;                // SPI帧长度为8bit
    SPI_InitStructure.Init.Direction = SPI_DIRECTION_1LINE;             // SPI单线模式
    SPI_InitStructure.Init.FirstBit = SPI_FIRSTBIT_MSB;                 // 高位在前
    SPI_InitStructure.Init.Mode = SPI_MODE_MASTER;                      // 工作在主机模式
    SPI_InitStructure.Init.NSS = SPI_NSS_SOFT;                          // 片选信号交给软件控制
    SPI_InitStructure.Init.TIMode = SPI_TIMODE_DISABLE;                 // 关闭TI模式
    HAL_SPI_Init(&SPI_InitStructure);
    __HAL_SPI_ENABLE(&SPI_InitStructure);
}

/**
 * @brief       端口初始化
 * @param       无
 * @retval      无
 */
void LCD_GPIOInit(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    
    LCD_RES_CLK_ENABLE();
    LCD_DC_CLK_ENABLE();
    LCD_CS_CLK_ENABLE();
    LCD_BLK_CLK_ENABLE();
    SPI1_Init();
    GPIO_InitStructure.Pin=LCD_RES_GPIO_PIN;
    GPIO_InitStructure.Mode=GPIO_MODE_OUTPUT_PP;
    GPIO_InitStructure.Speed=GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(LCD_RES_GPIO_PORT,&GPIO_InitStructure);
    HAL_GPIO_WritePin(LCD_RES_GPIO_PORT,LCD_RES_GPIO_PIN,GPIO_PIN_SET);
    
    GPIO_InitStructure.Pin=LCD_DC_GPIO_PIN;
    GPIO_InitStructure.Mode=GPIO_MODE_OUTPUT_PP;
    GPIO_InitStructure.Speed=GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(LCD_DC_GPIO_PORT,&GPIO_InitStructure);
    HAL_GPIO_WritePin(LCD_DC_GPIO_PORT,LCD_DC_GPIO_PIN,GPIO_PIN_SET);
    
    GPIO_InitStructure.Pin=LCD_CS_GPIO_PIN;
    GPIO_InitStructure.Mode=GPIO_MODE_OUTPUT_PP;
    GPIO_InitStructure.Speed=GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(LCD_CS_GPIO_PORT,&GPIO_InitStructure);
    HAL_GPIO_WritePin(LCD_CS_GPIO_PORT,LCD_CS_GPIO_PIN,GPIO_PIN_SET);
    
    GPIO_InitStructure.Pin=LCD_BLK_GPIO_PIN;
    GPIO_InitStructure.Mode=GPIO_MODE_OUTPUT_PP;
    GPIO_InitStructure.Speed=GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(LCD_BLK_GPIO_PORT,&GPIO_InitStructure);
    HAL_GPIO_WritePin(LCD_BLK_GPIO_PORT,LCD_BLK_GPIO_PIN,GPIO_PIN_SET);
}

/**
 * @brief       IO模拟SPI发送一个字节数据
 * @param       dat: 需要发送的字节数据
 * @retval      无
 */
void LCD_WR_Bus(uint8_t dat)
{
    LCD_CS_Clr();
    HAL_SPI_Transmit(&SPI_InitStructure,&dat,1,0xFF);
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
