#ifndef __BTIM_H
#define __BTIM_H

#include "./SYSTEM/sys/sys.h"
#include "./SYSTEM/delay/delay.h"

#define BTIM     TIM6
#define BTIM_CLK_ENABLE()     do { __HAL_RCC_TIM6_CLK_ENABLE(); }while(0)
#define BTIM_IRQn     TIM6_IRQn

void Btim_Init(uint32_t psc, uint32_t arr);

#endif
