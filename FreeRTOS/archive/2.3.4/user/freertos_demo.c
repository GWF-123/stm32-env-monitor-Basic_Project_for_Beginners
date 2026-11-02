#include "freertos_demo.h"
#include "./SYSTEM/usart/usart.h"
#include "./BSP/LED/led.h"
#include "./BSP/ADC/adc.h"
#include "./BSP/CMD/cmd.h"
#include "./APP/app_page_switch.h"
#include "./APP/app_voltage_oled.h"
#include "./APP/app_dht11_oled.h"
/*FreeRTOS*********************************************************************************************/
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "event_groups.h"
#include "timers.h"

/******************************************************************************************************/
/*FreeRTOS配置*/

/* START_TASK 任务 配置
 * 包括: 任务句柄 任务优先级 堆栈大小 创建任务
 */
#define START_TASK_PRIO 1                   /* 任务优先级 */
#define START_STK_SIZE  128                 /* 任务堆栈大小 */
TaskHandle_t            StartTask_Handler;  /* 任务句柄 */
void start_task(void *pvParameters);        /* 任务函数 */

/* TASK_COLLECT 采集+控制任务 配置
 * 干活: 等ADC DMA事件 → 算电压 → 阈值控制LED → 把快照发给显示任务
 */
#define COLLECT_TASK_PRIO   3               /* 任务优先级(最高: 数据不等人) */
#define COLLECT_STK_SIZE    192             /* 任务堆栈大小 */
TaskHandle_t            CollectTask_Handler;/* 任务句柄 */
void task_collect(void *pvParameters);      /* 任务函数 */

/* TASK_USART 串口命令任务 配置
 * 干活: 阻塞等一帧串口数据 → 解析命令 → 回复 → 重新开接收
 */
#define USART_TASK_PRIO     2               /* 任务优先级 */
#define USART_STK_SIZE      256             /* 任务堆栈大小(命令解析里有sprintf, 给宽裕点) */
TaskHandle_t            UsartTask_Handler;  /* 任务句柄 */
void task_usart(void *pvParameters);        /* 任务函数 */

/* TASK_DISPLAY 显示任务 配置
 * 干活: 取采集快照 → 扫键切页 → 刷新OLED当前页
 */
#define DISPLAY_TASK_PRIO   1               /* 任务优先级(UI最不急) */
#define DISPLAY_STK_SIZE    256             /* 任务堆栈大小(OLED刷屏+sprintf, 给宽裕点) */
TaskHandle_t            DisplayTask_Handler;/* 任务句柄 */
void task_display(void *pvParameters);      /* 任务函数 */

/* 队列: 采集任务 → 显示任务/串口命令 的数据快照
 * 长度1的邮箱: xQueueOverwrite 永远只留最新一份;
 * 多个读者(显示、串口READ_ADC)用xQueuePeek读, 看完不取走, 大家读到同一份数
 */
#define SAMPLE_QUEUE_LEN    1
QueueHandle_t           g_sample_queue;

typedef struct
{
    float v[ADC_CHY_NUM];           /* 三路滤波电压(电位/光敏/热敏) */
} SampleMsg_t;

/* 软件定时器: 每2秒触发一次DHT11读取(回调运行在定时器服务任务里) */
TimerHandle_t           g_dht11_timer;
static void Dht11_Timer_Callback(TimerHandle_t xTimer);

/* 阈值控制(从原main.c的while(1)里搬过来, 现在跟着采集任务跑) */
static uint8_t s_th_flag = 0;
static void Threshold_Value_Control(void);

/******************************************************************************************************/

/**
 * @brief       FreeRTOS例程入口函数
 * @param       无
 * @retval      无
 */
void freertos_demo(void)
{
    xTaskCreate((TaskFunction_t )start_task,            /* 任务函数 */
                (const char*    )"start_task",          /* 任务名称 */
                (uint16_t       )START_STK_SIZE,        /* 任务堆栈大小 */
                (void*          )NULL,                  /* 传入给任务函数的参数 */
                (UBaseType_t    )START_TASK_PRIO,       /* 任务优先级 */
                (TaskHandle_t*  )&StartTask_Handler);   /* 任务句柄 */
    vTaskStartScheduler();                              /* 启动调度器(不会返回) */
}

/**
 * @brief       start_task: 创建所有内核对象和任务, 然后删除自己
 * @param       pvParameters : 传入参数(未用到)
 * @retval      无
 */
void start_task(void *pvParameters)
{
    taskENTER_CRITICAL();           /* 进入临界区 */

    /* 内核对象 */
    g_adc_evt      = xEventGroupCreate();                                       /* 事件组: ADC DMA半满/全满 */
    g_sample_queue = xQueueCreate(SAMPLE_QUEUE_LEN, sizeof(SampleMsg_t));       /* 队列: 采集→显示 */
    g_dht11_timer  = xTimerCreate("dht11_tmr",                                  /* 软件定时器: DHT11定时读取 */
                                  pdMS_TO_TICKS(DHT11_REFRESH_MS),              /* 周期2s */
                                  pdTRUE,                                       /* 自动重装 */
                                  (void *)0,
                                  Dht11_Timer_Callback);

    /* 创建任务1: 采集+控制 */
    xTaskCreate((TaskFunction_t )task_collect,
                (const char*    )"task_collect",
                (uint16_t       )COLLECT_STK_SIZE,
                (void*          )NULL,
                (UBaseType_t    )COLLECT_TASK_PRIO,
                (TaskHandle_t*  )&CollectTask_Handler);
    /* 创建任务2: 串口命令 */
    xTaskCreate((TaskFunction_t )task_usart,
                (const char*    )"task_usart",
                (uint16_t       )USART_STK_SIZE,
                (void*          )NULL,
                (UBaseType_t    )USART_TASK_PRIO,
                (TaskHandle_t*  )&UsartTask_Handler);
    /* 创建任务3: 显示 */
    xTaskCreate((TaskFunction_t )task_display,
                (const char*    )"task_display",
                (uint16_t       )DISPLAY_STK_SIZE,
                (void*          )NULL,
                (UBaseType_t    )DISPLAY_TASK_PRIO,
                (TaskHandle_t*  )&DisplayTask_Handler);

    xTimerStart(g_dht11_timer, 0);  /* 启动软件定时器(命令发给定时器服务任务, 不阻塞) */

    vTaskDelete(StartTask_Handler); /* 删除开始任务 */
    taskEXIT_CRITICAL();            /* 退出临界区 */
}

/**
 * @brief       task_collect: 采集+控制任务
 * @param       pvParameters : 传入参数(未用到)
 * @retval      无
 */
void task_collect(void *pvParameters)
{
    SampleMsg_t msg;
    uint8_t i;

    while (1)
    {
        /* 等ADC DMA半满/全满事件(DMA中断里置位, 等到自动清零) */
        xEventGroupWaitBits(g_adc_evt,
                            ADC_EVT_HALF | ADC_EVT_FULL,
                            pdTRUE,         /* 等到后清掉事件位 */
                            pdFALSE,        /* 有一个就醒 */
                            portMAX_DELAY); /* 死等 */

        Adc_Calculate();                /* 处理半满/全满数据, 更新 adcx/filtered */
        Threshold_Value_Control();      /* 阈值判断 → LED1 */

        /* 打包快照发进邮箱(显示任务和串口READ_ADC都从邮箱读, 保证两边数值一致) */
        for (i = 0; i < ADC_CHY_NUM; i++)
        {
            msg.v[i] = filtered[i];
        }
        xQueueOverwrite(g_sample_queue, &msg);
    }
}

/**
 * @brief       读取最新采集快照(邮箱非破坏性peek)
 *              串口READ_ADC和OLED显示都用它: 两边取值时刻一致, 打印值才对得上屏上值
 *              peek看完不取走, 所以多个读者都能看同一份; 写侧xQueueOverwrite只留最新一份
 * @param       v : 输出三路电压(v[ADC_CHY_NUM])
 * @retval      1=拿到邮箱快照, 0=邮箱还没数据(兜底直接读filtered, 用在上电最初几毫秒)
 */
uint8_t Sample_GetLatest(float *v)
{
    SampleMsg_t msg;
    uint8_t i;

    if (xQueuePeek(g_sample_queue, &msg, 0) == pdTRUE)
    {
        for (i = 0; i < ADC_CHY_NUM; i++)
        {
            v[i] = msg.v[i];
        }
        return 1;
    }

    for (i = 0; i < ADC_CHY_NUM; i++)
    {
        v[i] = filtered[i];
    }
    return 0;
}

/**
 * @brief       task_usart: 串口命令任务
 * @param       pvParameters : 传入参数(未用到)
 * @retval      无
 */
void task_usart(void *pvParameters)
{
    uint8_t ok;

    while (1)
    {
        /* 二值信号量: 串口收满一帧(空闲中断)后由中断释放, 这里死等 */
        Uart_Rx_WaitFrame();

        ok = Cmd_Process_Line(g_rx_buffer, g_usart_rx_len);
        if (ok) Uart_Send_Dma("OK  \r\n");   /* 全部正确才输出 OK */

        g_usart_rx_flag = 0;
        HAL_UARTEx_ReceiveToIdle_DMA(&g_uart1_handle, g_rx_buffer, RXBUFFERSIZE);
    }
}

/**
 * @brief       task_display: 显示任务
 * @param       pvParameters : 传入参数(未用到)
 * @retval      无
 */
void task_display(void *pvParameters)
{
    float v[ADC_CHY_NUM];

    while (1)
    {
        /* 取最新采集快照(peek非破坏性读, 和串口READ_ADC读的是同一份; 没新数据沿用上一次) */
        if (Sample_GetLatest(v))
        {
            Voltage_UpdateData(v);
        }

        Page_Handle();                          /* 扫键 + 切页 + 刷新当前页 */

        vTaskDelay(pdMS_TO_TICKS(50));          /* 50ms一轮: 按键扫描和页面刷新的节奏 */
    }
}

/**
 * @brief       DHT11软件定时器回调(运行在定时器服务任务)
 * @param       xTimer : 触发的定时器句柄(未用到)
 * @retval      无
 */
static void Dht11_Timer_Callback(TimerHandle_t xTimer)
{
    (void)xTimer;
    /* 只置触发标志: DHT11一次读取约25ms, 不能在定时器服务任务里干重活,
       真正的读取和刷屏由显示任务完成 */
    DHT11_OLED_TrigRefresh();
}

/**
 * @brief       阈值控制(原main.c里的逻辑, 原样搬过来)
 * @param       无
 * @retval      无
 */
static void Threshold_Value_Control(void)
{
        /*
        只要采集值大于阈值，则点亮灯1
        */
        if(g_light_threshold < adcx[1])/*adcx[0]可以采集PA1来控制*/
        {
            LED1(0);
            s_th_flag = 1;
        }
        else if((g_light_threshold >= adcx[1]) && s_th_flag)
        {
            LED1(1);
            s_th_flag = 0;
        }
}
