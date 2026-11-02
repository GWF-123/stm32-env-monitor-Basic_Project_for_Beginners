#include "./APP/app_version_page.h"
#include "./BSP/OLED/bsp_i2c_oled.h"
#include "./BSP/FONTS/fonts.h"       

void Version_Show(void)
{   
    
    OLED_ShowChinese_F16X16(0, 1, 0);
    OLED_ShowChinese_F16X16(0, 2, 1);
    OLED_ShowChinese_F16X16(0, 3, 2);
    OLED_ShowChinese_F16X16(0, 4, 16);
    OLED_ShowChinese_F16X16(0, 5, 17);
    
    OLED_ShowChinese_F16X16(1, 1, 18);
    OLED_ShowChinese_F16X16(1, 2, 19);
    OLED_ShowString(2, 48, (uint8_t *)"DMA", TEXTSIZE_F8X16);
    OLED_ShowChinese(2, 72, 20, chinese_library_16x16);
    OLED_ShowChinese(2, 88, 21, chinese_library_16x16);
    OLED_ShowChinese_F16X16(2, 1, 22);
    OLED_ShowChinese_F16X16(2, 2, 23);
    OLED_ShowString_F8X16(2, 6, (uint8_t *)":1.2.0");
}
