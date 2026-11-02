#include "./BSP/ADC/adc.h"
#include "./BSP/DMA/dma.h"
#include "./SYSTEM/usart/usart.h"
#include <math.h>

#define VOL_DEADBAND    0.005f
#define FILTER_ALPHA    0.2f

static ADC_HandleTypeDef adc_handle = {0};

volatile uint16_t adcx[ADC_CHY_NUM] = {0};

static volatile uint16_t g_dma_buf[DMA_BUFFER];
float last_filtered[ADC_CHY_NUM] = {0};
float filtered[ADC_CHY_NUM] = {0};
static uint8_t first = 1;
volatile uint8_t g_dma_half_flag = 0;   /* 半传输标志：前半段满了 */
volatile uint8_t g_dma_full_flag = 0;   /* 传输完成标志：后半段满了 */

void Set_Adc_Channel(uint32_t ch, uint32_t rank, uint32_t stime)// ADC_REGULAR_RANK_1 ADC_SAMPLETIME_239CYCLES_5
{
    ADC_ChannelConfTypeDef adc_chconfig;
    
    adc_chconfig.Channel = ch;
    adc_chconfig.Rank = rank;
    adc_chconfig.SamplingTime = stime;
    
    HAL_ADC_ConfigChannel(&adc_handle, &adc_chconfig);
}

void Adc_Init(void)
{
    adc_handle.Instance = ADC_ADCX;
    adc_handle.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    adc_handle.Init.ContinuousConvMode = ENABLE;
    adc_handle.Init.ScanConvMode = ADC_SCAN_ENABLE;
    adc_handle.Init.DiscontinuousConvMode = DISABLE;
    adc_handle.Init.NbrOfConversion = ADC_CHY_NUM;
    adc_handle.Init.NbrOfDiscConversion = 0;
    adc_handle.Init.ExternalTrigConv = ADC_SOFTWARE_START;
    
    HAL_ADC_Init(&adc_handle);
    
    Set_Adc_Channel(ADC_ADCX_CHY1, ADC_REGULAR_RANK_1, ADC_SAMPLETIME_239CYCLES_5);
    Set_Adc_Channel(ADC_ADCX_CHY2, ADC_REGULAR_RANK_2, ADC_SAMPLETIME_239CYCLES_5);
    Set_Adc_Channel(ADC_ADCX_CHY3, ADC_REGULAR_RANK_3, ADC_SAMPLETIME_239CYCLES_5);
    
    __HAL_LINKDMA(&adc_handle, DMA_Handle, dma_adc_handle);
    
    HAL_ADCEx_Calibration_Start(&adc_handle);
    
    HAL_NVIC_SetPriority(DMA1_ADC_CHY_IRQN, 3, 3);
    HAL_NVIC_EnableIRQ(DMA1_ADC_CHY_IRQN);
    
   
    HAL_ADC_Start_DMA(&adc_handle, (uint32_t*)g_dma_buf, DMA_BUFFER);//循环模式下是最大的数值
    ;
}

void HAL_ADC_MspInit(ADC_HandleTypeDef* hadc)
{
    if(hadc -> Instance == ADC_ADCX)
    {
        ADC_CLK_ENABLE();
        ADC_GPIO_CLK_ENABLE();
        
        RCC_PeriphCLKInitTypeDef rcc_struct;
        
        rcc_struct.PeriphClockSelection = RCC_PERIPHCLK_ADC;
        rcc_struct.AdcClockSelection = RCC_ADCPCLK2_DIV6;
        
        HAL_RCCEx_PeriphCLKConfig(&rcc_struct);
        
        GPIO_InitTypeDef gpio_init = {0};
        
        gpio_init.Pin = ADC_GPIO_PIN;
        gpio_init.Mode = GPIO_MODE_ANALOG;
        
        HAL_GPIO_Init(ADC_GPIO_PORT, &gpio_init);
    }
}

//搬运好了进入此中断
void DMA1_ADC_CHY_IRQH(void)
{
    /* 传输完成中断（整帧写完） */
    if (__HAL_DMA_GET_FLAG(&dma_adc_handle, DMA_FLAG_TC1))
    {
        g_dma_full_flag = 1;
        __HAL_DMA_CLEAR_FLAG(&dma_adc_handle, DMA_FLAG_TC1);
    }

    /* 半传输中断（写到一半） */
    if (__HAL_DMA_GET_FLAG(&dma_adc_handle, DMA_FLAG_HT1))
    {
        g_dma_half_flag = 1;
        __HAL_DMA_CLEAR_FLAG(&dma_adc_handle, DMA_FLAG_HT1);
    }

}

uint32_t Adc_Get_Average_Half(uint32_t ch, uint8_t half)
{
    uint16_t n = DMA_BUFFER / ADC_CHY_NUM / 2;   /* 每半段每通道的采样数 */
    uint16_t buf[64];   /* 临时数组，注意大小要够（DMA_BUFFER/(3*2)） */
    uint16_t tmp;
    uint8_t i, j;
    uint32_t sum = 0;
    uint16_t start_idx;
    
    /* 计算起始偏移：
     * 前半段起始 = ch                              (第 1 组的第 ch 个)
     * 后半段起始 = DMA_BUFFER / 2 + ch            (后半段第 1 组的第 ch 个)
     */
    start_idx = (half == 0) ? ch : (DMA_BUFFER / 2 + ch);

    /* 把这半段里该通道的所有采样点取出来 */
    for (i = 0; i < n; ++i)
    {
        buf[i] = g_dma_buf[start_idx + i * ADC_CHY_NUM];
    }
    
    for(i = 0;i < n - 1; ++i)
    {
        for(j = 0; j < n - i - 1; ++j)
        {
            if(buf[j] > buf[j + 1])
            {
                tmp = buf[j + 1];
                buf[j + 1] = buf[j];
                buf[j] = tmp;
            }
        }
    }
    
    for(i = 1; i < n - 1; ++i)
    {
        sum += buf[i];
    }
    
    return (uint16_t)(sum / (n - 2));
}

void Adc_Calculate(void)
{
    
    uint8_t i;
    float voltage = 0.0f;
    uint8_t data_ready = 0;   /* 这一轮有没有新数据 */
    
    /* ---- 有半传输中断，处理前半段 ---- */
    if (g_dma_half_flag)
    {
        g_dma_half_flag = 0;
        for (i = 0; i < ADC_CHY_NUM; ++i)
        {
            adcx[i] = Adc_Get_Average_Half(i, 0);  /* 前半段 */
        }
        data_ready = 1;
    }
    
    /* ---- 有全传输中断，处理后半段 ---- */
    if (g_dma_full_flag)
    {
        g_dma_full_flag = 0;
        for (i = 0; i < ADC_CHY_NUM; ++i)
        {
            adcx[i] = Adc_Get_Average_Half(i, 1);  /* 后半段 */
        }
        data_ready = 1;
    }
    
        /* ---- 没有新数据就直接返回 ---- */
    if (!data_ready) return;
    
    for(i = 0; i < ADC_CHY_NUM; ++i)
    {
        voltage = (float)adcx[i] * (3.3f / 4096.0f);
        
        if(first)
        {
            filtered[i] = voltage;
            last_filtered[i] = filtered[i];
        }
        else
        {
            filtered[i] += FILTER_ALPHA * (voltage - filtered[i]);
            if(first || fabsf(filtered[i] - last_filtered[i]) >= VOL_DEADBAND)
            {
                last_filtered[i] = filtered[i];
            }
        }
    }
    
    first = 0;
    
}

