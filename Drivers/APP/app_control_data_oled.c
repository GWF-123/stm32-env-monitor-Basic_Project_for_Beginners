#include "./APP/app_control_data_oled.h"
#include "./BSP/OLED/bsp_i2c_oled.h"        /* OLED基础显示接口 */
#include "./BSP/FONTS/fonts.h"              /* 8x16字库 */
#include "./APP/app_voltage_oled.h"         /* 第1页：电压显示（调用你已有的函数） */
#include "./BSP/ADC/adc.h"
#include "./SYSTEM/usart/usart.h"
#include "./BSP/CMD/cmd.h"
#include <stdio.h>

static uint16_t last_light_t = LIGHT_TH_DEFAULT;
static uint16_t last_pwm_t = PWM_DEFAULT;
static const uint8_t val_clear[32] = {0};

#define DIGITAL_WIDTH 32


static void ClearValueArea(uint8_t line)
{
    uint8_t page = line * 2;
    OLED_SetPos(page, 48);
    OLED_WriteBuffer(OLED_WR_DATA, (uint8_t *)val_clear, DIGITAL_WIDTH);
    OLED_SetPos(page + 1, 48);
    OLED_WriteBuffer(OLED_WR_DATA, (uint8_t *)val_clear, DIGITAL_WIDTH);
}

/* 只显示某一行的数值 */
static void Show_Value(uint8_t line, uint16_t v)
{
    char buf[7];
    
    /* 用sprintf拼接成字符串，如 "2.710V" */
    sprintf(buf, "%04d", v);

    /* 8x16大字体显示数值 */
    OLED_ShowString(line, 48, (uint8_t *)buf, TEXTSIZE_F8X16);
}

void Chinese_Obv(void)
{
    //控制面板文字
    OLED_ShowChinese_F16X16(0, 2, 12);
    OLED_ShowChinese_F16X16(0, 3, 13);
    OLED_ShowChinese_F16X16(0, 4, 14);
    OLED_ShowChinese_F16X16(0, 5, 15);
    
    //
    OLED_ShowString(2, 0, (uint8_t *)"LIGHT:", TEXTSIZE_F8X16);
    OLED_ShowString(4, 0, (uint8_t *)"PWM:", TEXTSIZE_F8X16);
    
    Show_Value(2, g_light_threshold);
    Show_Value(4, g_pwm_duty);
}

void Control_Data(void)
{
    if(last_light_t != g_light_threshold)
    {
        last_light_t = g_light_threshold;
        ClearValueArea(1);
        Show_Value(2, g_light_threshold);
    }
    
    if(last_pwm_t != g_pwm_duty)
    {
        last_pwm_t = g_pwm_duty;
        ClearValueArea(2);
        Show_Value(4, g_pwm_duty);
    }
}

