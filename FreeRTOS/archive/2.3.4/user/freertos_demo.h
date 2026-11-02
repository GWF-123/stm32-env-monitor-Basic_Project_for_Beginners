#ifndef __FREERTOS_DEMO_H
#define __FREERTOS_DEMO_H

#include "./SYSTEM/sys/sys.h"

void freertos_demo(void);   /* FreeRTOS入口: 创建启动任务并启动调度器(不会返回) */

/* 读取最新采集快照(邮箱peek, 看完不取走): 串口READ_ADC和OLED显示都用它, 两边读同一份数 */
uint8_t Sample_GetLatest(float *v);   /* v[]输出三路电压; 返回1=拿到快照, 0=还没数据(兜底读filtered) */

#endif
