#include "./SYSTEM/sys/sys.h"
#include "./SYSTEM/usart/usart.h"
#include "./SYSTEM/delay/delay.h"
#include "./BSP/DMA/dma.h"
#include "./BSP/ADC/adc.h"
#include "./BSP/PWM/pwm.h"
#include "./BSP/CMD/cmd.h"
#include "./BSP/LED/led.h"
#include "./BSP/KEY/key.h"
#include "./BSP/IIC/myiic.h"
#include "./BSP/OLED/bsp_i2c_oled.h"
#include "./APP/app_voltage_oled.h"
#include "./APP/app_page_switch.h"
#include <stdio.h>

int main(void)
{
    HAL_Init();                               
    sys_stm32_clock_init(RCC_PLL_MUL9);//时钟
    delay_init(72);                          
    usart_init(115200);
    //初始化区域
    printf("OLED实验：\r\n");
    Adc_Init();
    Iic_Init();
    OLED_Init();
    Cmd_Init();
    Pwm_Init(72 - 1, 1000 - 1);
    Key_Init(); /* ← 加这一行：按键初始化 */ 
    Page_Init(); /* ← 加这一行：页面管理初始化（默认第1页） */
   
    
    //while函数
    while(1)
    {
        if (g_usart_rx_flag)
        {
            uint8_t ok = Cmd_Process_Line(g_rx_buffer, g_usart_rx_len);
            if (ok) printf("OK\r\n");   /* 全部正确才输出 OK */

            g_usart_rx_flag = 0;
            //uart_send_dma(g_rx_buffer, g_usart_rx_len);
            HAL_UARTEx_ReceiveToIdle_DMA(&g_uart1_handle, g_rx_buffer, RXBUFFERSIZE);
        }
        
        Page_Handle();

    }
    
}

