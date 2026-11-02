#include "./APP/app_page_switch.h"
#include "./BSP/KEY/key.h"                  /* 按键扫描 */
#include "./BSP/OLED/bsp_i2c_oled.h"        /* OLED基础显示接口 */
#include "./BSP/FONTS/fonts.h"              /* 8x16字库 */
#include "./APP/app_voltage_oled.h"         /* 第1页：电压显示（调用你已有的函数） */
#include "./SYSTEM/delay/delay.h"
#include "./APP/app_control_data_oled.h"
#include "./APP/app_version_page.h"
#include <stdio.h>

/* ================================================================================
 *  说明：完全在基础代码之上扩展，不修改任何基础文件。
 *  功能：KEY0 → 下一页，KEY1 → 上一页，页面循环（第1页按KEY1跳到第3页）
 *  页面：
 *     第1页 PAGE_VOLTAGE — 电压值（调用 Voltage_ShowLabels / Voltage_RefreshValues）
 *     第2页 PAGE_NUM2    — 显示一个大数字 "2"（供你后续自己改）
 *     第3页 PAGE_NUM3    — 显示一个大数字 "3"（供你后续自己改）
 * ================================================================================ */

/* 当前页码（静态变量，只在本文件内有效） */
static uint8_t current_page = PAGE_VOLTAGE;

/* 页面是否刚切换（需要整屏重绘）的标志 */
static uint8_t page_just_changed = 1;

/* ================================================================================
 *  整屏清屏 + 画当前页的全部静态内容
 *  只在切页的瞬间调用一次，平时不调用
 * ================================================================================ */
static void DrawPage_Static(uint8_t page)
{
    OLED_Fill(0x00);   /* 先整屏清 */

    switch (page)
    {
        case PAGE_VOLTAGE:
            /* 第1页：电压 —— 调用你已有的标签显示函数 */
            Voltage_ShowLabels();
            break;

        case PAGE_NUM2:
            /* 第2页：显示阈值 */
            Chinese_Obv();
            break;

        case PAGE_NUM3:
            /* 第3页：显示版本  */
            Version_Show();
            break;

        default:
            break;
    }
}


/* ================================================================================
 *  当前页的动态刷新（数值变化才重绘的部分）
 *  主循环里反复调用
 * ================================================================================ */
static void RefreshPage_Dynamic(uint8_t page)
{
    switch (page)
    {
        case PAGE_VOLTAGE:
            /* 第1页：刷新电压数值（只刷变化的那一路） */
            Voltage_RefreshValues();
            break;

        case PAGE_NUM2:
            Control_Data();
            break;
        case PAGE_NUM3:
            /* 第2、3页是静态数字，不需要动态刷新，什么都不做 */
            break;

        default:
            break;
    }
}


/* ================================================================================
 *  切页：下一页（KEY0）
 *  循环：第1→2→3→1
 * ================================================================================ */
static void Page_Next(void)
{
    current_page++;
    if (current_page > PAGE_TOTAL)
        current_page = 1;
    page_just_changed = 1;
}


/* ================================================================================
 *  切页：上一页（KEY1）
 *  循环：第1→3→2→1
 * ================================================================================ */
static void Page_Prev(void)
{
    if (current_page == 1)
        current_page = PAGE_TOTAL;
    else
        current_page--;
    page_just_changed = 1;
}


/* ================================================================================
 *  1. 页面管理初始化（在 OLED_Init 之后调用一次）
 * ================================================================================ */
void Page_Init(void)
{
    current_page = PAGE_VOLTAGE;   /* 默认第1页 */
    page_just_changed = 1;         /* 标记需要整屏重绘 */
}


/* ================================================================================
 *  2. 页面总处理函数（主循环里反复调用）
 *     功能：① 扫键 → 切页   ② 切页则整屏重绘静态内容   ③ 动态刷新当前页
 * ================================================================================ */
void Page_Handle(void)
{
    uint8_t key;

    /* ---- ① 按键扫描（不支持连续按，mode=0，按下松开才算一次） ---- */
    key = key_scan(0);

    if (key == KEY0_PRES)        /* KEY0 = 下一页 */
    {
        Page_Next();
    }
    else if (key == KEY1_PRES)   /* KEY1 = 上一页 */
    {
        Page_Prev();
    }

    /* ---- ② 刚切页 → 整屏重绘该页的静态内容 ---- */
    if (page_just_changed)
    {
        DrawPage_Static(current_page);
        page_just_changed = 0;
    }

    /* ---- ③ 动态刷新当前页（只有第1页有数值要刷，其他页空转） ---- */
    RefreshPage_Dynamic(current_page);
}

/*****************************END OF FILE***************************************/
