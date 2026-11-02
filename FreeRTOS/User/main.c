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
#include "./BSP/DHT11/dht11.h"
#include "./APP/app_voltage_oled.h"
#include "./APP/app_page_switch.h"
# include "./APP/app_save.h"
#include "./MALLOC/malloc.h"
#include "freertos_demo.h"
#include <stdio.h>

void Read_Norflash_Data(uint8_t save_ok);//把上电读取保存的数据

int main(void)
{
    HAL_Init();
    sys_stm32_clock_init(RCC_PLL_MUL9);//时钟
    delay_init(72);
    Usart_Init(115200);
    //初始化区域
    printf("智能监测终端：\r\n");
    Norflash_Init();
    Led_Init();
    Adc_Init();
    Iic_Init();
    OLED_Init();
    Dht11_Init();
    Cmd_Init();
    Pwm_Init(72 - 1, 1000 - 1);
    Key_Init();
    Page_Init();
    my_mem_init(SRAMIN);            /* 初始化内部SRAM内存池(STM32F103ZET6无外扩SRAM, 只用SRAMIN) */

    /*上电读取norflash*/
    uint8_t save_ok = Save_Init(&g_save_data);
    Read_Norflash_Data(save_ok);

    freertos_demo();                /* 创建启动任务并启动调度器(不会返回) */
}

void Read_Norflash_Data(uint8_t save_ok)
{
    if (save_ok)
    {
        /* 1 恢复光敏阈值 */
        g_light_threshold = g_save_data.light_val;

        /* 2 恢复 PWM 占空比 + 设置到定时器硬件 */
        g_pwm_duty = g_save_data.pwm_val;
        Led_Control(g_pwm_duty);

        /* 3 恢复页码（Page_SetCurrent 内部会标记需要重绘） */
        Page_SetCurrent(g_save_data.page_num);


        static char buf[128]; //防止栈内存被优化掉
        sprintf (buf, "上电恢复成功：light=%d pwm=%d page=%d\r\n",
               g_light_threshold, g_pwm_duty, g_save_data.page_num);
        Uart_Send_Dma(buf);
    }
    else
    {
        Uart_Send_Dma("上电恢复失败（第一次上电？），使用默认值\r\n");
    }
}
