#ifndef __APP_DHT11_OLED_H
#define __APP_DHT11_OLED_H

#include "./SYSTEM/sys/sys.h"

#define DHT11_REFRESH_MS      2000   /* 温湿度刷新间隔：2000ms = 2秒 */
#define DHT11_LINE_TITLE      0      
#define DHT11_LINE_TEMP       1      
#define DHT11_LINE_HUMI       2      
#define DHT11_LINE_TIP        3      

void DHT11_OLED_ShowLabels(void);          /* 显示温湿度页的静态文字（标题/标签/提示，只调用一次） */
void DHT11_OLED_Refresh(void);             /* 主循环反复调用，内部计时，到2秒自动刷新数值 */
void DHT11_OLED_ResetTimer(void);          /* 切到本页时调用，重置计时器（马上触发一次刷新） */

#endif 
