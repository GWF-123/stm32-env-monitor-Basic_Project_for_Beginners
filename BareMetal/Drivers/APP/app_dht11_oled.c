#include "./APP/app_dht11_oled.h"
#include "./BSP/OLED/bsp_i2c_oled.h"
#include "./BSP/DHT11/dht11.h"
#include "./BSP/FONTS/fonts.h"
#include <stdio.h>
#include <string.h>


/************************ 静态变量 ************************/

static uint32_t g_last_refresh_tick = 0;   /* 上次刷新的时间戳（ms） */
static uint8_t  g_sensor_ok = 0;           /* 传感器是否正常（1=正常，0=未检测到） */
static uint8_t  g_prev_ok = 99;            /* 上一次的状态（99=初始值，第一次强制重绘） */
static uint8_t  g_temperature = 0;         /* 温度（整数，单位℃） */
static uint8_t  g_humidity = 0;            /* 湿度（整数，单位%RH） */


/************************ 内部辅助函数声明 ************************/

static void ClearLineFull(uint8_t line);
static void ClearLineArea(uint8_t line, uint8_t x, uint8_t col_cnt);
static void ShowTempHumiLabels(void);
static void ShowTempHumiValues(void);
static void ShowNotFound(void);


/************************ 对外函数 ************************/

/**
 * @brief  显示温湿度页的静态文字（标题、底部提示）
 * @note   切到温湿度页时调用一次，数值由 Refresh 刷新
 */
void DHT11_OLED_ShowLabels(void)
{
    /* ===== 第0行：标题"温湿度检测"（5个汉字，居中） =====
     * 5字 × 16px = 80px，(128-80)/2 = 24px 偏移
     */
    OLED_ShowChinese(DHT11_LINE_TITLE, 24, 28, chinese_library_16x16);   /* 温 */
    OLED_ShowChinese(DHT11_LINE_TITLE, 40, 29, chinese_library_16x16);   /* 湿 */
    OLED_ShowChinese(DHT11_LINE_TITLE, 56, 30, chinese_library_16x16);   /* 度 */
    OLED_ShowChinese(DHT11_LINE_TITLE, 72, 32, chinese_library_16x16);   /* 检 */
    OLED_ShowChinese(DHT11_LINE_TITLE, 88, 33, chinese_library_16x16);   /* 测 */

    /* ===== 第3行：底部提示"两秒刷新一次数值"（8个汉字，铺满128px） ===== */
    OLED_ShowChinese(DHT11_LINE_TIP * 2, 0,   35, chinese_library_16x16);   /* 两 */
    OLED_ShowChinese(DHT11_LINE_TIP * 2, 16,  36, chinese_library_16x16);   /* 秒 */
    OLED_ShowChinese(DHT11_LINE_TIP * 2, 32,  37, chinese_library_16x16);   /* 刷 */
    OLED_ShowChinese(DHT11_LINE_TIP * 2, 48,  38, chinese_library_16x16);   /* 新 */
    OLED_ShowChinese(DHT11_LINE_TIP * 2, 64,  39, chinese_library_16x16);   /* 一 */
    OLED_ShowChinese(DHT11_LINE_TIP * 2, 80,  40, chinese_library_16x16);   /* 次 */
    OLED_ShowChinese(DHT11_LINE_TIP * 2, 96,  41, chinese_library_16x16);   /* 数 */
    OLED_ShowChinese(DHT11_LINE_TIP * 2, 112, 42, chinese_library_16x16);   /* 值 */

    /* 中间两行先清空 */
    ClearLineFull(DHT11_LINE_TEMP);
    ClearLineFull(DHT11_LINE_HUMI);

    /* 强制第一次刷新时根据状态整行重绘 */
    g_prev_ok = 99;
}

/**
 * @brief  主循环反复调用，内部计时，到2秒自动读取并刷新显示
 * @note   用 HAL_GetTick() 做时间基准，非阻塞
 *         状态变化时整行重绘（保证无残留），状态不变时只刷数值区（减少闪烁）
 */
void DHT11_OLED_Refresh(void)
{
    uint32_t now = HAL_GetTick();
    uint8_t  temp, humi;
    uint8_t  ret;

    /* 还没到刷新时间，直接返回 */
    if (now - g_last_refresh_tick < DHT11_REFRESH_MS)
    {
        return;
    }

    /* 到时间了，记录这次的时间戳 */
    g_last_refresh_tick = now;

    /* 读取 DHT11 */
    ret = Dht11_Read_Data(&temp, &humi);

    if (ret == 0)
    {
        g_sensor_ok = 1;
        g_temperature = temp;
        g_humidity    = humi;

        /* 状态变化了（从未检测到 → 检测到），先整行清，再画标签+数值 */
        if (g_prev_ok != 1)
        {
            ClearLineFull(DHT11_LINE_TEMP);
            ClearLineFull(DHT11_LINE_HUMI);
            ShowTempHumiLabels();
            g_prev_ok = 1;
        }

        ShowTempHumiValues();   /* 只刷新数值区 */
    }
    else
    {
        g_sensor_ok = 0;

        /* 状态变化了（从检测到 → 未检测到），整行清了再画 */
        if (g_prev_ok != 0)
        {
            ClearLineFull(DHT11_LINE_TEMP);
            ClearLineFull(DHT11_LINE_HUMI);
            g_prev_ok = 0;
        }

        ShowNotFound();
    }
}

/**
 * @brief  重置刷新计时器
 * @note   切到温湿度页时调用，让它马上读一次，不用等2秒
 */
void DHT11_OLED_ResetTimer(void)
{
    g_last_refresh_tick = 0;   /* 设为0，下次Refresh时 now-0 肯定大于2000，马上刷新 */
    g_prev_ok = 99;            /* 强制状态切换，保证切回来时整行重绘干净 */
}


/************************ 内部辅助函数 ************************/

/**
 * @brief  清除一整行（128像素全清）
 * @param  line  行号（0~3），对应页 line*2 ~ line*2+1
 */
static void ClearLineFull(uint8_t line)
{
    uint8_t buf[16];
    memset(buf, 0, 16);   /* 16字节0，但我们要写128列，所以循环8次 */

    /* 上半页也就是1行， 128位，先从左到右，再自动下移一行 */
    OLED_SetPos(line * 2, 0);
    OLED_WriteBuffer(OLED_WR_DATA, buf, 16);
    OLED_WriteBuffer(OLED_WR_DATA, buf, 16);
    OLED_WriteBuffer(OLED_WR_DATA, buf, 16);
    OLED_WriteBuffer(OLED_WR_DATA, buf, 16);
    OLED_WriteBuffer(OLED_WR_DATA, buf, 16);
    OLED_WriteBuffer(OLED_WR_DATA, buf, 16);
    OLED_WriteBuffer(OLED_WR_DATA, buf, 16);
    OLED_WriteBuffer(OLED_WR_DATA, buf, 16);
    /*把buf改成128，否则读到垃圾值，上面这么写是为了节省栈内存
    OLED_WriteBuffer(OLED_WR_DATA, buf, 128);
    */

    /* 下半页 */
    OLED_SetPos(line * 2 + 1, 0);
    OLED_WriteBuffer(OLED_WR_DATA, buf, 16);
    OLED_WriteBuffer(OLED_WR_DATA, buf, 16);
    OLED_WriteBuffer(OLED_WR_DATA, buf, 16);
    OLED_WriteBuffer(OLED_WR_DATA, buf, 16);
    OLED_WriteBuffer(OLED_WR_DATA, buf, 16);
    OLED_WriteBuffer(OLED_WR_DATA, buf, 16);
    OLED_WriteBuffer(OLED_WR_DATA, buf, 16);
    OLED_WriteBuffer(OLED_WR_DATA, buf, 16);
}

/**
 * @brief  清除某一行指定宽度的区域（用来清旧数值）
 * @param  line     行号（0~3）
 * @param  x        起始列（像素，0~127）
 * @param  col_cnt  清除多少列（像素数）
 */
static void ClearLineArea(uint8_t line, uint8_t x, uint8_t col_cnt)
{
    uint8_t buf[16];
    uint8_t remaining = col_cnt;

    memset(buf, 0, 16);

    /* 上半页 */
    OLED_SetPos(line * 2, x);
    while (remaining > 0)
    {
        uint8_t chunk = (remaining > 16) ? 16 : remaining;
        OLED_WriteBuffer(OLED_WR_DATA, buf, chunk);
        remaining -= chunk;
    }

    /* 下半页 */
    remaining = col_cnt;
    OLED_SetPos(line * 2 + 1, x);
    while (remaining > 0)
    {
        uint8_t chunk = (remaining > 16) ? 16 : remaining;
        OLED_WriteBuffer(OLED_WR_DATA, buf, chunk);
        remaining -= chunk;
    }
}

/**
 * @brief  画温度行和湿度行的标签（"温度："和"湿度："）
 * @note   只在状态从"无"变"有"时调用一次
 */
static void ShowTempHumiLabels(void)
{
    /* 温度行的标签 "温度：" */
    OLED_ShowChinese(DHT11_LINE_TEMP * 2, 24, 28, chinese_library_16x16);    /* 温 */
    OLED_ShowChinese(DHT11_LINE_TEMP * 2, 40, 30, chinese_library_16x16);    /* 度 */
    OLED_ShowChar(DHT11_LINE_TEMP * 2, 56, ':', TEXTSIZE_F8X16);         /* 冒号 */

    /* 湿度行的标签 "湿度：" */
    OLED_ShowChinese(DHT11_LINE_HUMI * 2, 24, 29, chinese_library_16x16);    /* 湿 */
    OLED_ShowChinese(DHT11_LINE_HUMI * 2, 40, 30, chinese_library_16x16);    /* 度 */
    OLED_ShowChar(DHT11_LINE_HUMI * 2, 56, ':', TEXTSIZE_F8X16);         /* 冒号 */
}

/**
 * @brief  只刷新温度和湿度的数值部分（标签不动）
 * @note   传感器正常、状态没变化时调用，减少闪烁
 */
static void ShowTempHumiValues(void)
{
    char buf[16];

    /* ===== 温度数值：xx.x℃ === */
    ClearLineArea(DHT11_LINE_TEMP, 64, 56);
    sprintf(buf, "%d.0", g_temperature);
    OLED_ShowString(DHT11_LINE_TEMP * 2, 64, (uint8_t *)buf, 8);
    /* ℃ 符号（字库索引27，16x16大小） */
    OLED_ShowChinese(DHT11_LINE_TEMP * 2, 64 + 5 * 8, 27, chinese_library_16x16);

    /* ===== 湿度数值：xx.x%RH =====*/
    ClearLineArea(DHT11_LINE_HUMI, 64, 56);
    sprintf(buf, "%d.0%%RH", g_humidity);
    OLED_ShowString(DHT11_LINE_HUMI * 2, 64, (uint8_t *)buf, 8);
}

/**
 * @brief  显示"未检测到DHT11"（传感器不存在时调用）
 * @note   在第1行居中显示
 */
static void ShowNotFound(void)
{
    OLED_ShowChinese(DHT11_LINE_TEMP * 2, 12, 31, chinese_library_16x16);   /* 未 */
    OLED_ShowChinese(DHT11_LINE_TEMP * 2, 28, 32, chinese_library_16x16);   /* 检 */
    OLED_ShowChinese(DHT11_LINE_TEMP * 2, 44, 33, chinese_library_16x16);   /* 测 */
    OLED_ShowChinese(DHT11_LINE_TEMP * 2, 60, 34, chinese_library_16x16);   /* 到 */
    OLED_ShowString(DHT11_LINE_TEMP * 2, 76, (uint8_t *)"DHT11", 8);
}
