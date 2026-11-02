#include "./BSP/DMA/dma.h"

DMA_HandleTypeDef dma_rx_handle;
DMA_HandleTypeDef dma_tx_handle;
DMA_HandleTypeDef dma_adc_handle;

static uint8_t dma_initialized = 0;   /* 【修复】防止 Dma_Init 被重复调用 */

void Dma_Init(void)
{
    if (dma_initialized) return;   /* 已经初始化过就直接返回, 防止状态机被搞乱 */
    dma_initialized = 1;

    DMA1_CLK_ENABLE();

    /* ========== ADC DMA 通道1 ========== */
    dma_adc_handle.Instance = DMA1_ADC_CHY;
    dma_adc_handle.Init.Direction = DMA_PERIPH_TO_MEMORY;
    dma_adc_handle.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
    dma_adc_handle.Init.PeriphInc = DMA_PINC_DISABLE;
    dma_adc_handle.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
    dma_adc_handle.Init.MemInc = DMA_MINC_ENABLE;
    dma_adc_handle.Init.Mode = DMA_NORMAL;
    dma_adc_handle.Init.Priority = DMA_PRIORITY_HIGH;
    HAL_DMA_Init(&dma_adc_handle);

    /* ========== 串口 RX DMA 通道5 ========== */
    dma_rx_handle.Instance = DMA1_RX_CHY;
    dma_rx_handle.Init.Direction = DMA_PERIPH_TO_MEMORY;
    dma_rx_handle.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    dma_rx_handle.Init.PeriphInc = DMA_PINC_DISABLE;
    dma_rx_handle.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    dma_rx_handle.Init.MemInc = DMA_MINC_ENABLE;
    dma_rx_handle.Init.Mode = DMA_NORMAL;
    dma_rx_handle.Init.Priority = DMA_PRIORITY_HIGH;
    HAL_DMA_Init(&dma_rx_handle);

    /* ========== 串口 TX DMA 通道4 ========== */
    dma_tx_handle.Instance = DMA1_TX_CHY;
    dma_tx_handle.Init.Direction = DMA_MEMORY_TO_PERIPH;
    dma_tx_handle.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    dma_tx_handle.Init.PeriphInc = DMA_PINC_DISABLE;
    dma_tx_handle.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    dma_tx_handle.Init.MemInc = DMA_MINC_ENABLE;
    dma_tx_handle.Init.Mode = DMA_NORMAL;
    dma_tx_handle.Init.Priority = DMA_PRIORITY_HIGH;
    HAL_DMA_Init(&dma_tx_handle);

    /* ========== 中断配置 ========== */
    HAL_NVIC_SetPriority(DMA1_ADC_CHY_IRQN, 3, 3);
    HAL_NVIC_EnableIRQ(DMA1_ADC_CHY_IRQN);

    HAL_NVIC_SetPriority(DMA1_RX_CHY_IRQN, 2, 2);
    HAL_NVIC_EnableIRQ(DMA1_RX_CHY_IRQN);

    HAL_NVIC_SetPriority(DMA1_TX_CHY_IRQN, 3, 3);
    HAL_NVIC_EnableIRQ(DMA1_TX_CHY_IRQN);
}

void DMA1_TX_CHY_IRQH(void)
{
    HAL_DMA_IRQHandler(&dma_tx_handle);
}

void DMA1_RX_CHY_IRQH(void)
{
    HAL_DMA_IRQHandler(&dma_rx_handle);
}
