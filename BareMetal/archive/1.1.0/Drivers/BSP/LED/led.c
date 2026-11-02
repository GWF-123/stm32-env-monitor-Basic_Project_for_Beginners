#include "./BSP/LED/led.h"

void Led_Init(void)
{
    GPIO_InitTypeDef gpio_struct = {0};
    
    LED1_CLK_ENABLE();
    LED0_CLK_ENABLE();
    
    gpio_struct.Pin = LED1_PIN;
    gpio_struct.Mode = GPIO_MODE_OUTPUT_PP;
    gpio_struct.Pull = GPIO_NOPULL;
    gpio_struct.Speed = GPIO_SPEED_FREQ_MEDIUM;
     
    HAL_GPIO_Init(LED1_PORT,&gpio_struct);
    
//    gpio_struct.Pin = LED0_PIN;
//    gpio_struct.Mode = GPIO_MODE_OUTPUT_PP;
//    gpio_struct.Pull = GPIO_NOPULL;
//    gpio_struct.Speed = GPIO_SPEED_FREQ_MEDIUM;
//     
//    HAL_GPIO_Init(LED0_PORT,&gpio_struct);
    
     LED1(1);//接高电平，默认不亮
     //LED0(1);
}

