#ifndef __APP_VOLTAGE_OLED_H
#define __APP_VOLTAGE_OLED_H

#include "./SYSTEM/sys/sys.h"


/* 函数声明 */
void Voltage_ShowLabels(void);        /* 一次性显示静态文字：三路标签+冒号（只调用一次） */
void Voltage_RefreshValues(void);     /* 只刷新三路电压数值（标题、标签不动） */

/* 说明：
 *  标题"多通道系统采集"在 main 中画（第0行），不属于刷新范围；
 *  三路电压标签： 第1行 电位 | 第2行 光敏 | 第3行 热敏（均固定不刷新）
 *  只有数值部分会随 ADC 数据变化刷新。
 */

#endif /* __APP_VOLTAGE_OLED_H */
