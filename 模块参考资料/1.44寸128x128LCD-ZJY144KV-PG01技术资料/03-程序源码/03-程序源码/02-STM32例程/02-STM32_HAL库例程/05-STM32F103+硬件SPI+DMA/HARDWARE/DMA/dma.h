#ifndef _DMA_H_
#define _DMA_H_

#include "spi.h"

void MYDMA_Config(DMA_Channel_TypeDef*DMA_CHx);//≈‰÷√DMA1_CHx
void MYDMA_Config1(DMA_Channel_TypeDef* DMA_CHx);//≈‰÷√DMA1_CHx
void MYDMA_Enable(uint32_t SrcAddress, uint32_t DstAddress, uint32_t DataLength);

#endif



