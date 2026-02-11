#ifndef _DMA_H_
#define _DMA_H_
#include "sys.h"
#include "stm32f10x_dma.h"

void MYDMA_Config(DMA_Channel_TypeDef *DMA_CHx, uint32_t cpar, uint32_t cmar, uint16_t cndtr); // ≈‰÷√DMA1_CHx
void MYDMA_Config1(DMA_Channel_TypeDef *DMA_CHx, uint32_t cpar, uint32_t cmar, uint16_t cndtr);
void MYDMA_Enable(DMA_Channel_TypeDef *DMA_CHx); //  πƒ‹DMA1_CHx

#endif







