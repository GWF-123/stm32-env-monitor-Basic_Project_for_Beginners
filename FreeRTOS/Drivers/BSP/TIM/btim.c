#include "./BSP/TIM/btim.h"
#include "./BSP/LED/led.h"
#include "./APP/led_control.h"
#include "./BSP/PWM/pwm.h"
#include "./BSP/ADC/adc.h"

static TIM_HandleTypeDef btim_handle = {0};

void Btim_Init(uint32_t psc, uint32_t arr)
{
    
    //GPIO_InitTypeDef gpio_struct = {0};
    BTIM_CLK_ENABLE();
    
    btim_handle.Instance = BTIM;
    btim_handle.Init.Prescaler = psc;
    btim_handle.Init.CounterMode = TIM_COUNTERMODE_UP;
    btim_handle.Init.Period = arr;
    HAL_TIM_Base_Init(&btim_handle);
    
    
    
    HAL_NVIC_SetPriority(BTIM_IRQn, 1, 3);
    HAL_NVIC_EnableIRQ(BTIM_IRQn);
    
    HAL_TIM_Base_Start_IT(&btim_handle);

}

void TIM6_DAC_IRQHandler(void)
{
    HAL_TIM_IRQHandler(&btim_handle);
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if(htim -> Instance == BTIM)
    {
        Adc_Calculate();
    }
    
}


