#ifndef __APP_PAGE_SWITCH_H
#define __APP_PAGE_SWITCH_H

#include "./SYSTEM/sys/sys.h"

/* 总页数 */
#define PAGE_TOTAL   3

/* 页面编号 */
#define PAGE_VOLTAGE  1   /* 第1页：电压值 */
#define PAGE_NUM2     2   /* 第2页：显示数字2 */
#define PAGE_NUM3     3   /* 第3页：显示数字3 */


/* 函数声明 */
void Page_Init(void);          /* 页面管理初始化（进入第1页） */
void Page_Handle(void);        /* 页面处理（主循环调用，内部扫键+切页+刷新当前页） */

#endif /* __APP_PAGE_SWITCH_H */

