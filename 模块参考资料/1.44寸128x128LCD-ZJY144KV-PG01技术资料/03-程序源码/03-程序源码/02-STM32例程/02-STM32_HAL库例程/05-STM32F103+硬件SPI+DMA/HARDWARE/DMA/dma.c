#include "dma.h"

DMA_HandleTypeDef DMA_InitStructure;
extern SPI_HandleTypeDef SPI_InitStructure;

void MYDMA_Config(DMA_Channel_TypeDef *DMA_CHx)
{
    __HAL_RCC_DMA1_CLK_ENABLE();                                  // 使能DMA时钟
    __HAL_LINKDMA(&SPI_InitStructure, hdmatx, DMA_InitStructure); // 将SPI外设与DMA链接起来

    DMA_InitStructure.Instance = DMA_CHx;                             // 配置DMA通道
    DMA_InitStructure.Init.Direction = DMA_MEMORY_TO_PERIPH;          // 数据传输方向从内存发送到外设
    DMA_InitStructure.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;    // 存储器数据长度为8bit
    DMA_InitStructure.Init.MemInc = DMA_MINC_ENABLE;                  // 存储器地址自增
    DMA_InitStructure.Init.Mode = DMA_NORMAL;                         // 普通模式
    DMA_InitStructure.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE; // 外设数据长度为8bit
    DMA_InitStructure.Init.PeriphInc = DMA_PINC_DISABLE;              // 外设地址不变
    DMA_InitStructure.Init.Priority = DMA_PRIORITY_MEDIUM;            // DMA通道拥有中优先级
    HAL_DMA_DeInit(&DMA_InitStructure);                               // 取消配置DMA重置一下
    HAL_DMA_Init(&DMA_InitStructure);                                 // 初始化DMA外设
}
void MYDMA_Config1(DMA_Channel_TypeDef *DMA_CHx)
{
    __HAL_RCC_DMA1_CLK_ENABLE();                                  // 使能DMA时钟
    __HAL_LINKDMA(&SPI_InitStructure, hdmatx, DMA_InitStructure); // 将SPI外设与DMA链接起

    DMA_InitStructure.Instance = DMA_CHx;                                 // 配置DMA通道
    DMA_InitStructure.Init.Direction = DMA_MEMORY_TO_PERIPH;              // 数据传输方向从内存发送到外设
    DMA_InitStructure.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;    // 存储器数据长度为16bit
    DMA_InitStructure.Init.MemInc = DMA_MINC_DISABLE;                     // 存储器地址不变
    DMA_InitStructure.Init.Mode = DMA_NORMAL;                             // 普通模式
    DMA_InitStructure.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD; // 外设数据长度为16bit
    DMA_InitStructure.Init.PeriphInc = DMA_PINC_DISABLE;                  // 外设地址不变
    DMA_InitStructure.Init.Priority = DMA_PRIORITY_MEDIUM;                // DMA通道拥有中优先级
    HAL_DMA_DeInit(&DMA_InitStructure);                                   // 取消配置DMA重置一下
    HAL_DMA_Init(&DMA_InitStructure);                                     // 初始化DMA外设
}
/******************************************************************************
函数说明：使能一次DMA发送
入口数据：SrcAddress  The source memory Buffer address      数据源地址
          DstAddress  The destination memory Buffer address 目标地址
          DataLength  发送数据长度最大65535
返回值：  无
******************************************************************************/
void MYDMA_Enable(uint32_t SrcAddress, uint32_t DstAddress, uint32_t DataLength)
{
    __HAL_DMA_DISABLE(&DMA_InitStructure);
    DMA_InitStructure.Instance->CPAR = DstAddress;
    DMA_InitStructure.Instance->CMAR = SrcAddress;             // 定义
    DMA_InitStructure.Instance->CNDTR = DataLength;            // 定义数据传输量
    SET_BIT(SPI_InitStructure.Instance->CR2, SPI_CR2_TXDMAEN); // 开启SPI TX DMA请求
    __HAL_DMA_ENABLE(&DMA_InitStructure);
}
