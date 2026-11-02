#ifndef __APP_DHT11_OLED_H
#define __APP_DHT11_OLED_H

#include "./SYSTEM/sys/sys.h"


/************************ 宏定义 ************************/

#define DHT11_REFRESH_MS      2000   /* 温湿度刷新间隔：2000ms = 2秒 */
#define DHT11_LINE_TITLE      0      /* 标题行（第0行，对应页0~1） */
#define DHT11_LINE_TEMP       1      /* 温度行（第1行，对应页2~3） */
#define DHT11_LINE_HUMI       2      /* 湿度行（第2行，对应页4~5） */
#define DHT11_LINE_TIP        3      /* 提示行（第3行，对应页6~7） */


/************************ 函数声明 ************************/

void DHT11_OLED_ShowLabels(void);          /* 显示温湿度页的静态文字（标题/标签/提示，只调用一次） */
void DHT11_OLED_Refresh(void);             /* 主循环反复调用，内部计时，到2秒自动刷新数值 */
void DHT11_OLED_ResetTimer(void);          /* 切到本页时调用，重置计时器（马上触发一次刷新） */


#endif /* __APP_DHT11_OLED_H */
