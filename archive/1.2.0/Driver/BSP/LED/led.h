#ifndef __LED_H
#define __LED_H

#include "./SYSTEM/sys/sys.h"

#define LED1_PORT   GPIOE
#define LED1_PIN  GPIO_PIN_5
#define LED1_CLK_ENABLE()   do{ __HAL_RCC_GPIOE_CLK_ENABLE();}while(0)

#define LED0_PORT   GPIOB
#define LED0_PIN  GPIO_PIN_5
#define LED0_CLK_ENABLE()   do{ __HAL_RCC_GPIOB_CLK_ENABLE();}while(0)

#define LIGHTNESS 999

//在:前面不需要加;类似int b = a ? 1 : 0;1的后面需要加;一样
#define LED1(x) do { x ? \
                    HAL_GPIO_WritePin(LED1_PORT, LED1_PIN, GPIO_PIN_SET) :\
                    HAL_GPIO_WritePin(LED1_PORT, LED1_PIN, GPIO_PIN_RESET); \
                    }while(0)

#define LED0(x) do { x ? \
                    HAL_GPIO_WritePin(LED0_PORT, LED0_PIN, GPIO_PIN_SET) :\
                    HAL_GPIO_WritePin(LED0_PORT, LED0_PIN, GPIO_PIN_RESET); \
                    }while(0)

#define LED1_TOGGLE() do { HAL_GPIO_TogglePin(LED1_PORT, LED1_PIN); }while(0)
#define LED0_TOGGLE() do { HAL_GPIO_TogglePin(LED0_PORT, LED0_PIN); }while(0)
                    
void Led_Init(void);

#endif
