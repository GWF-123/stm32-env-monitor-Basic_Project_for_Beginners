#include "./APP/app_version_page.h"
#include "./BSP/OLED/bsp_i2c_oled.h"
#include "./BSP/FONTS/fonts.h"       

void Version_Show(void)
{   
    
    OLED_ShowChinese_F16X16(1, 0, 43);
    OLED_ShowChinese_F16X16(1, 1, 44);
    OLED_ShowChinese_F16X16(1, 2, 45);
    OLED_ShowChinese_F16X16(1, 3, 46);
    OLED_ShowChinese_F16X16(1, 4, 47);
    OLED_ShowChinese_F16X16(1, 5, 48);
    OLED_ShowChinese_F16X16(1, 6, 49);
    OLED_ShowChinese_F16X16(1, 7, 50);
    
    OLED_ShowChinese_F16X16(2, 1, 22);
    OLED_ShowChinese_F16X16(2, 2, 23);
    OLED_ShowString_F8X16(2, 6, (uint8_t *)":1.3.3");
}
