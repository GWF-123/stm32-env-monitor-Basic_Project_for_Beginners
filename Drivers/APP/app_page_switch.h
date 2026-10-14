#ifndef __APP_PAGE_SWITCH_H
#define __APP_PAGE_SWITCH_H

#include "./SYSTEM/sys/sys.h"

/* 总页数 */
#define PAGE_TOTAL   4

/* 页面编号 各页显示不同功能*/
#define PAGE_VOLTAGE  1   
#define PAGE_CONT     2   /*控制*/
#define PAGE_DHT11   3      
#define PAGE_VER     4   /*版本*/


extern uint8_t current_page;
/* 函数声明 */
void Page_Init(void);          /* 页面管理初始化（进入第1页） */
void Page_Handle(void);        /* 页面处理（主循环调用，内部扫键+切页+刷新当前页） */
void DrawPage_Static(uint8_t page);
void Page_SetCurrent(uint8_t page);

#endif /* __APP_PAGE_SWITCH_H */

