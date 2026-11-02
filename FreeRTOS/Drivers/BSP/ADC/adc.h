#ifndef __ADC_H
#define __ADC_H

#include "./SYSTEM/sys/sys.h"
#include "FreeRTOS.h"               
#include "event_groups.h"           /* 事件组类型用 */

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

/* 事件组事件位: ADC DMA半满/全满(DMA中断里置位, 采集任务里等待) */
#define ADC_EVT_HALF    (1 << 0)    /* DMA搬到一半(前半段满了) */
#define ADC_EVT_FULL    (1 << 1)    /* DMA搬完一圈(后半段满了) */
extern EventGroupHandle_t g_adc_evt;    /* 事件组句柄(在freertos_demo.c里创建) */

extern volatile uint16_t adcx[ADC_CHY_NUM];
extern float filtered[ADC_CHY_NUM];
void Adc_Init(void);
void Adc_Calculate(void);

#endif 
