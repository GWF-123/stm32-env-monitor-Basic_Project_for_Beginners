#include "./BSP/PWM/pwm.h"
#include "./BSP/LED/led.h"

static TIM_HandleTypeDef tim_handle = {0};

void Pwm_Init(uint16_t psc, uint16_t arr)
{
    GPIO_InitTypeDef gpio_struct = {0};
    
    __HAL_RCC_AFIO_CLK_ENABLE();
    __HAL_AFIO_REMAP_TIM3_PARTIAL();
    __HAL_RCC_TIM3_CLK_ENABLE();//记得使用时钟(写仔细复用)
    
    tim_handle.Instance = PWM_TIM;
    tim_handle.Init.Prescaler = psc;
    tim_handle.Init.CounterMode = TIM_COUNTERMODE_UP;
    tim_handle.Init.Period = arr;
    HAL_TIM_PWM_Init(&tim_handle);

    LED0_CLK_ENABLE();

    gpio_struct.Pin = LED0_PIN;
    gpio_struct.Mode = GPIO_MODE_AF_PP;
    gpio_struct.Pull = GPIO_NOPULL;
    gpio_struct.Speed = GPIO_SPEED_FREQ_MEDIUM;
     
    HAL_GPIO_Init(LED0_PORT, &gpio_struct);
    
    TIM_OC_InitTypeDef pwm_oc_init = {0};
    pwm_oc_init.OCMode = TIM_OCMODE_PWM1;
    pwm_oc_init.Pulse = 0;
    pwm_oc_init.OCPolarity = TIM_OCPOLARITY_LOW;
    
    HAL_TIM_PWM_ConfigChannel(&tim_handle, &pwm_oc_init, PWM_CHANNAL);
    
    HAL_TIM_PWM_Start(&tim_handle, PWM_CHANNAL);
}

void Led_Control(uint16_t light)
{
    __HAL_TIM_SET_COMPARE(&tim_handle, PWM_CHANNAL, light);
}
