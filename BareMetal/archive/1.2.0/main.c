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
#include "./BSP/SPI/spi.h"
#include "./BSP/NORFLASH/norflash.h"
#include "./BSP/MAX30102/app_max30102_simple.h"
#include "./APP/app_voltage_oled.h"
#include "./APP/app_page_switch.h"
# include "./APP/app_save.h"
#include <stdio.h>

int main(void)
{
    HAL_Init();                               
    sys_stm32_clock_init(RCC_PLL_MUL9);//时钟
    delay_init(72);                          
    usart_init(115200);
    //初始化区域
//    printf("掉电读取数据实验：\r\n");
//    Norflash_Init();
//    Adc_Init();
//    Iic_Init();
//    OLED_Init();
//    Cmd_Init();
//    Pwm_Init(72 - 1, 1000 - 1);
//    Key_Init(); /* ← 加这一行：按键初始化 */ 
//    Page_Init(); /* ← 加这一行：页面管理初始化（默认第1页） */
//    Save_Init(&g_save_data);
//    Save_DebugPrint();
    printf("掉电读取数据实验：\r\n");
    Norflash_Init();
    Adc_Init();
    Iic_Init();
    OLED_Init();
    Cmd_Init();
    Pwm_Init(72 - 1, 1000 - 1);
    Key_Init();
    Page_Init();
    
    uint8_t save_ok = Save_Init(&g_save_data);

    Save_DebugPrint();   /* 调试打印，确认 Flash 里的数据（以后可以注释掉） */

    if (save_ok)
    {
        /* ① 恢复光敏阈值 */
        g_light_threshold = g_save_data.light_val;

        /* ② 恢复 PWM 占空比 + 设置到定时器硬件 */
        g_pwm_duty = g_save_data.pwm_val;
        Led_Control(g_pwm_duty);

        /* ③ 恢复页码（Page_SetCurrent 内部会标记需要重绘） */
        Page_SetCurrent(g_save_data.page_num);

        printf("上电恢复成功：light=%d pwm=%d page=%d\r\n",
               g_light_threshold, g_pwm_duty, g_save_data.page_num);
    }
    else
    {
        printf("上电恢复失败（第一次上电？），使用默认值\r\n");
    }
    
    //while函数
    while(1)
    {
        
        if (g_usart_rx_flag)
        {
            uint8_t ok = Cmd_Process_Line(g_rx_buffer, g_usart_rx_len);
            if (ok) printf("OK\r\n");   /* 全部正确才输出 OK */

            g_usart_rx_flag = 0;
            HAL_UARTEx_ReceiveToIdle_DMA(&g_uart1_handle, g_rx_buffer, RXBUFFERSIZE);
        }
        
        Page_Handle();
        
    }
    
}

