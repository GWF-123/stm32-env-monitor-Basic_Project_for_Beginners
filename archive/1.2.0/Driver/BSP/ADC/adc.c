#include "./BSP/DMA/dma.h"
#include "./BSP/ADC/adc.h"
#include "./SYSTEM/usart/usart.h"
#include <math.h>

#define VOL_DEADBAND    0.005f
#define FILTER_ALPHA    0.2f

static ADC_HandleTypeDef adc_handle = {0};

uint16_t adcx[ADC_CHY_NUM] = {0};
volatile uint8_t g_dma_flag = 0;

static volatile uint16_t g_dma_buf[DMA_BUFFER];
float last_filtered[ADC_CHY_NUM] = {0};
float filtered[ADC_CHY_NUM] = {0};
static uint8_t first = 1;

void Set_Adc_Channel(uint32_t ch, uint32_t rank, uint32_t stime)// ADC_REGULAR_RANK_1 ADC_SAMPLETIME_239CYCLES_5
{
    ADC_ChannelConfTypeDef adc_chconfig;
    
    adc_chconfig.Channel = ch;
    adc_chconfig.Rank = rank;
    adc_chconfig.SamplingTime = stime;
    
    HAL_ADC_ConfigChannel(&adc_handle, &adc_chconfig);
}

void Dma_Enable_Cndtr(uint16_t cndtr)
{
    __HAL_DMA_DISABLE(&dma_adc_handle);
    __HAL_ADC_DISABLE(&adc_handle);
    
     while (DMA1_ADC_CHY->CCR & DMA_CCR_EN) { }//     DMA1->IFCR = DMA_FLAG_GL1 | DMA_FLAG_TC1 | DMA_FLAG_HT1 | DMA_FLAG_TE1;

    DMA1_ADC_CHY -> CNDTR = cndtr;
    
    //先启动DMA再启动START
    __HAL_DMA_ENABLE(&dma_adc_handle);
    __HAL_ADC_ENABLE(&adc_handle);
    HAL_ADC_Start(&adc_handle);
    
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
    
    HAL_DMA_Start_IT(&dma_adc_handle, (uint32_t)&ADC_ADCX ->DR, (uint32_t)g_dma_buf, 0);
    HAL_ADC_Start_DMA(&adc_handle, (uint32_t*)g_dma_buf, 0);
    
    Dma_Enable_Cndtr(DMA_BUFFER);
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

void DMA1_ADC_CHY_IRQH(void)
{
    if(DMA1 ->ISR & DMA_FLAG_TC1)
    {
        g_dma_flag = 1;
        DMA1 ->IFCR |= DMA_FLAG_TC1;
        //__HAL_DMA_CLEAR_FLAG(&dma_handle, DMA_FLAG_TC1);
    }

}

uint32_t Adc_Get_Average(uint32_t ch)
{
    uint16_t n = DMA_BUFFER / ADC_CHY_NUM;
    uint16_t buf[DMA_BUFFER / ADC_CHY_NUM], tmp;
    uint8_t i, j;
    uint32_t sum = 0;
    
    for(i = 0; i < n; ++i)
    {
        buf[i] = g_dma_buf[i * ADC_CHY_NUM + ch];
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
    if(!g_dma_flag)
    {
        return ;
    }
    
    //uint8_t changed = 0; 
    uint8_t i;
    float voltage = 0.0f;
    
    for(i = 0; i < ADC_CHY_NUM; ++i)
    {
        adcx[i] = Adc_Get_Average(i);
    }
    
    for(i = 0; i < ADC_CHY_NUM; ++i)
    {
        voltage = (float)adcx[i] * (3.3f / 4096.0f);
        
        if(first)
        {
            filtered[i] = voltage;
            last_filtered[i] = filtered[i];
            //changed = 1;
        }
        else
        {
            filtered[i] += FILTER_ALPHA * (voltage - filtered[i]);
            if(first || fabsf(filtered[i] - last_filtered[i]) >= VOL_DEADBAND)
            {
                last_filtered[i] = filtered[i];
                //changed = 1;
            }
        }
    }
    
    first = 0;
    g_dma_flag = 0;
    Dma_Enable_Cndtr(DMA_BUFFER);
    
//    if(changed)
//    {
//        printf("ADC_CH1 : %4d  %.3fV, ADC_CH2 : %4d  %.3fV, ADC_CH3 : %4d  %.3fV\r\n",
//        adcx[0], filtered[0],adcx[1], filtered[1],adcx[2], filtered[2]
//        );
//    }
    
}

