#include "./APP/app_voltage_oled.h"
#include "./BSP/ADC/adc.h"           /* 电压数据来源：filtered[] */
#include "./BSP/OLED/bsp_i2c_oled.h" /* OLED显示接口 + ascii_code_8x16 字库 */
#include "./BSP/FONTS/fonts.h"       /* ascii_code_8x16 声明 */
#include "./SYSTEM/delay/delay.h"
#include <stdio.h>                   /* sprintf */
#include <math.h>

/* ================================================================================
 *  说明：完全在基础代码之上扩展，不修改任何基础文件。
 *  电压数据：来自 adc.c 的 filtered[]（单位V）
 *     filtered[0 ~ 3] = 可自行接传感器
 *  刷新策略（只刷新变化的那一路，变化小于阈值不刷新）：
 *    - 用 last_v[3] 保存上一次显示的电压值
 *    - 每次读取新值后，逐路与上一次比较
 *    - 只有当变化量 > CHANGE_THRESHOLD 时，才重绘那一路的数值
 * ================================================================================ */

/* 每行数值的起始列像素：标签(16x16中文，48px) + 冒号(8px) 后从第56列开始 */
#define VAL_X       56
/* 数值最大宽度： "X.XXXV" 为6个8x16字符，共 6*8=48 像素 */
#define VAL_WIDTH   48

/* 变化阈值（单位V）：变化量小于这个值，认为没变化，不刷新
 * 0.01f = 10mV，ADC有微小抖动时不会频繁重绘，数字更稳定
 * 如果你想更灵敏，改小一点（比如 0.005f = 5mV） */
#define CHANGE_THRESHOLD  0.01f

/* 电压路数：电位、光敏、热敏 */
#define VOLTAGE_CH_NUM    3

/* ---- 数值区域清零缓冲（值域约48像素宽，占2页） ---- */
static const uint8_t val_clear[VAL_WIDTH] = {0};

/* 上一次显示的电压值（初始值设为一个不可能的值，保证第一次一定刷新） */
static float last_v[VOLTAGE_CH_NUM] = {-999.0f, -999.0f, -999.0f};


/* 用8x16字库显示字符串
 * @param line : 文本行号（1~3），每行占2页（16像素高）
 * @param x_pix: 起始列像素坐标
 * @param str  : 字符串（通过 ascii_code_8x16 显示任意可见ASCII字符）
 */
static void OLED_ShowBigString(uint8_t line, uint8_t x_pix, uint8_t *str)
{
    uint8_t page = line * 2;
    uint8_t col  = x_pix;

    while (*str != '\0')
    {
        OLED_ShowChar(page, col, *str, TEXTSIZE_F8X16);  /* 一次画上下两页 */
        col += 8;   /* 8x16字符，宽8像素 */
        str++;
    }
}


/* 清空某一行的数值区域（第像素，上下两页），不动标签和冒号 16 * 3 + 8 = 56, x.xxxV 6 * 8 = 48*/
static void ClearValueArea(uint8_t line)
{
    uint8_t page = line * 2;
    OLED_SetPos(page, VAL_X);
    OLED_WriteBuffer(OLED_WR_DATA, (uint8_t *)val_clear, VAL_WIDTH);
    OLED_SetPos(page + 1, VAL_X);
    OLED_WriteBuffer(OLED_WR_DATA, (uint8_t *)val_clear, VAL_WIDTH);
}


/* 只显示某一行的电压数值（整数/小数分离，3位小数，sprintf拼接，8x16显示） */
static void ShowOneValue(uint8_t line, float v)
{
    char buf[10];
    int int_part, frac_part;

    /* 分离整数部分和小数部分，取小数点后3位（四舍五入） */
    int_part  = (int)v;                                 /* 整数部分 */
    frac_part = (int)((v - int_part) * 1000.0f + 0.5f); /* 小数部分 */

    if (frac_part >= 1000)   /* 进位保护，如 2.9996 → 3.000 */
    {
        frac_part = 0;
        int_part++;
    }

    /* 用sprintf拼接成字符串，如 "2.710V" */
    sprintf(buf, "%d.%03dV", int_part, frac_part);

    /* 8x16大字体显示数值 */
    OLED_ShowBigString(line, VAL_X, (uint8_t *)buf);
}


/* ================================================================================
 *  1. 一次性显示静态文字：三路标签+冒号（在 OLED_Init() 之后只调用一次）
 *     布局：           1~3行布局3通道采集电压
 * ================================================================================ */
void Voltage_ShowLabels(void)
{
    //多通道采集系统文字显示
    OLED_ShowChinese_F16X16(0, 0, 0);  
    OLED_ShowChinese_F16X16(0, 1, 1);  
    OLED_ShowChinese_F16X16(0, 2, 2);  
    OLED_ShowChinese_F16X16(0, 3, 3);  
    OLED_ShowChinese_F16X16(0, 4, 4);
    OLED_ShowChinese_F16X16(0, 5, 5);
    OLED_ShowChinese_F16X16(0, 6, 6);
    
    /* 第1行 */
    OLED_ShowChinese_F16X16(1, 0, 1);
    OLED_ShowChinese_F16X16(1, 1, 2);
    OLED_ShowChinese_F16X16(1, 2, 24);
    OLED_ShowChar(1 * 2, 16 * 3, ':', TEXTSIZE_F8X16);//

    /* 第2行： */
    OLED_ShowChinese_F16X16(2, 0, 1);
    OLED_ShowChinese_F16X16(2, 1, 2);
    OLED_ShowChinese_F16X16(2, 2, 25);
    OLED_ShowChar(2 * 2, 16 * 3, ':', TEXTSIZE_F8X16);

    /* 第3行：*/
    OLED_ShowChinese_F16X16(3, 0, 1);
    OLED_ShowChinese_F16X16(3, 1, 2);
    OLED_ShowChinese_F16X16(3, 2, 26);
    OLED_ShowChar(3 * 2, 16 * 3, ':', TEXTSIZE_F8X16);

    /* 清空三路数值区域，保证首次显示只有标签 */
    ClearValueArea(1);
    ClearValueArea(2);
    ClearValueArea(3);
    
     /* 把上一次值重置，保证首次调用 Voltage_RefreshValues 一定刷新 */
    last_v[0] = -999.0f;
    last_v[1] = -999.0f;
    last_v[2] = -999.0f;
}


/* ================================================================================
 *  2. 只刷新三路电压数值（标题、标签、冒号固定不动）
 *     在主循环中反复调用，只重绘数值区域，无闪烁
* line:0~3
 * ================================================================================ */
void Voltage_RefreshValues(void)
{
    uint8_t i;
    float diff;
    
    /* 刷新ADC数据（内部有DMA标志判断，未就绪会自动跳过） */
    Adc_Calculate();

    /* 三路独立判断：哪一路变化了，就只刷新哪一路 */
    for (i = 0; i < VOLTAGE_CH_NUM; i++)
    {
        diff = fabsf(filtered[i] - last_v[i]);   /* 计算变化量绝对值 */

        if (diff > CHANGE_THRESHOLD)             /* 只有变化超过阈值才刷新 */
        {
            ClearValueArea(i + 1);               /* 清旧值（行号 = 索引+1） */
            ShowOneValue(i + 1, filtered[i]);    /* 画新值 */
            last_v[i] = filtered[i];             /* 记录这次的值，供下次比较 */
        }
    }
}

/*****************************END OF FILE***************************************/
