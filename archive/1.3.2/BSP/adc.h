#ifndef __ADC_H
#define __ADC_H

#include "./SYSTEM/sys/sys.h"

#define ADC_ADCX ADC1
#define ADC_ADCX_CHY1   ADC_CHANNEL_1
#define ADC_ADCX_CHY2   ADC_CHANNEL_2
#define ADC_ADCX_CHY3   ADC_CHANNEL_3

#define ADC_GPIO_PIN    GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3
#define ADC_GPIO_PORT   GPIOA
#define ADC_CHY_NUM     3

#define ADC_CLK_ENABLE()    do{ __HAL_RCC_ADC1_CLK_ENABLE(); }while(0)
#define ADC_GPIO_CLK_ENABLE()   do{ __HAL_RCC_GPIOA_CLK_ENABLE(); }while(0)

#define DMA_BUFFER      50 * ADC_CHY_NUM

extern volatile uint16_t adcx[ADC_CHY_NUM];
extern float filtered[ADC_CHY_NUM];
void Adc_Init(void);
void Adc_Calculate(void);

#endif 
