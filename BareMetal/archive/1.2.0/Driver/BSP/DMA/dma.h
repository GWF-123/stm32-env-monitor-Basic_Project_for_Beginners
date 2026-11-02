#ifndef __DMA_H
#define __DMA_H

#include "./SYSTEM/sys/sys.h"

#define DMA1_ADC_CHY    DMA1_Channel1
#define DMA1_RX_CHY     DMA1_Channel5
#define DMA1_TX_CHY     DMA1_Channel4
#define DMA1_CLK_ENABLE()     do{ __HAL_RCC_DMA1_CLK_ENABLE(); } while(0)

#define DMA1_RX_CHY_IRQN   DMA1_Channel5_IRQn
#define DMA1_RX_CHY_IRQH   DMA1_Channel5_IRQHandler
#define DMA1_ADC_CHY_IRQN  DMA1_Channel1_IRQn
#define DMA1_ADC_CHY_IRQH  DMA1_Channel1_IRQHandler

#define DMA1_TX_CHY_IRQN   DMA1_Channel4_IRQn
#define DMA1_TX_CHY_IRQH   DMA1_Channel4_IRQHandler

void Dma_Init(void);
extern DMA_HandleTypeDef dma_rx_handle;
extern DMA_HandleTypeDef dma_tx_handle;
extern DMA_HandleTypeDef dma_adc_handle;

#endif
