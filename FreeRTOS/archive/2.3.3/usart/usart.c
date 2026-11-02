#include "./SYSTEM/sys/sys.h"
#include "./SYSTEM/usart/usart.h"
#include "./BSP/DMA/dma.h"
#include <string.h>                                     /* ==========【新增】memcpy用 ========== */
#include <stdarg.h>                                     /* ==========【新增】Uart_Printf的可变参数用 ========== */

/* 使用FreeRTOS: 串口中断→任务用二值信号量, 发送互斥用互斥量 */
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

/* 加入以下代码, 支持printf函数, 而不需要选择use MicroLIB   */

#if 1

#if (__ARMCC_VERSION >= 6010050)            /* 使用AC6编译器时 */
__asm(".global __use_no_semihosting\n\t");  /* 声明不使用半主机模式 */
__asm(".global __ARM_use_no_argv \n\t");    /* AC6下需要声明main函数为无参数格式，否则部分例程可能出现半主机模式 */

#else
/* 使用AC5编译器时, 要在这里定义__FILE 和 不使用半主机模式 */
#pragma import(__use_no_semihosting)

struct __FILE
{
    int handle;
};

#endif

int _ttywrch(int ch)
{
    ch = ch;
    return ch;
}

void _sys_exit(int x)
{
    x = x;
}

char *_sys_command_string(char *cmd, int len)
{
    return NULL;
}

FILE __stdout;

/* MDK下需要重定义fputc函数, printf函数最终会通过调用fputc输出字符串到串口
 * 寄存器级操作, 比HAL_UART_Transmit更直接, 任何HAL版本都能用
 * 注意: fputc一个字符一锁没法防多任务穿插输出, 多任务下请用Uart_Printf(整串互斥) */
int fputc(int ch, FILE *f)
{
    while ((USART_UX->SR & 0X40) == 0);     /* 等待上一个字符发送完成 */

    USART_UX->DR = (uint8_t)ch;             /* 将要发送的字符 ch 写入到DR寄存器 */
    return ch;
}
#endif

#if USART_EN_RX /*如果使能了接收*/

volatile uint16_t g_usart_rx_flag = 0;      /* 1 = 收到一帧 */
volatile uint16_t g_usart_rx_len = 0;       /* 本帧实际字节数 */
volatile uint8_t  g_usart_tx_busy = 0;

static uint8_t g_tx_queue[TX_QUEUE_SIZE];
static volatile uint16_t g_tx_head = 0 ; /* 写指针 */
static volatile uint16_t g_tx_tail = 0 ; /* 读指针(DMA发) */
static volatile uint8_t g_tx_busy = 0 ; /* DMA 是否在发送 */
static volatile TickType_t g_tx_start_tick = 0 ; /* 本次DMA的启动时刻, 用于发送超时兜底 */

/* DMA发送超时阈值(ms)
 * g_tx_busy 只能靠发送完成中断清, 那个中断一旦丢一次, 之后所有发送都会被判忙
 * 静默丢弃, 而且完全没有报错, 现象跟硬件接触不良几乎一样.
 * 115200下512字节约45ms, 9600下约533ms, 1000ms足够宽松;
 * 以后如果把波特率降到4800以下, 这个值要跟着放大 */
#define TX_TIMEOUT_MS                   1000

/*启动DMA发送的三件套先声明, 因为发送完成中断回调在下面会用到 */
static void Uart_Start_Dma_Send_Impl(TickType_t now); /* 只做判忙+启动, 不加锁 */
static void Uart_Start_Dma_Send(void);                /* 任务态/调度器未启动时调用 */
static void Uart_Start_Dma_Send_FromISR(void);        /* 中断态调用 */
static void Uart_Tx_Recover_If_Timeout(void);         /* R2: 发送超时兜底 */

/* ==========FreeRTOS同步对象 ========== */
SemaphoreHandle_t g_usart_rx_sem = NULL;    /* 二值信号量: 收满一帧后中断释放, 任务里阻塞等 */
SemaphoreHandle_t g_tx_mutex = NULL;        /* 互斥量: 串口发送互斥, 防多任务混着发 */

uint8_t g_rx_buffer[RXBUFFERSIZE];          /* HAL库底层接收缓冲(一帧数据先落在这里) */

UART_HandleTypeDef g_uart1_handle;          /* UART句柄 */

void Usart_Init(uint32_t baudrate)
{
    /* 先建好同步对象再开中断, 防止中断先来时对象还是空的 */
    g_usart_rx_sem = xSemaphoreCreateBinary();
    g_tx_mutex = xSemaphoreCreateMutex();

    Dma_Init();

    g_uart1_handle.Instance = USART_UX;
    g_uart1_handle.Init.BaudRate = baudrate;
    g_uart1_handle.Init.WordLength = UART_WORDLENGTH_8B;
    g_uart1_handle.Init.StopBits = UART_STOPBITS_1;
    g_uart1_handle.Init.Parity = UART_PARITY_NONE;
    g_uart1_handle.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    g_uart1_handle.Init.Mode = UART_MODE_TX_RX;
    HAL_UART_Init(&g_uart1_handle);                                           /* HAL_UART_Init()会使能UART1 */

    HAL_UARTEx_ReceiveToIdle_DMA(&g_uart1_handle, g_rx_buffer, RXBUFFERSIZE);
}

void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{
    GPIO_InitTypeDef gpio_init_struct;

    if (huart->Instance == USART_UX)                            /* 如果是串口1，进行串口1 MSP初始化 */
    {
        USART_TX_GPIO_CLK_ENABLE();                             /* 使能串口TX脚时钟 */
        USART_RX_GPIO_CLK_ENABLE();                             /* 使能串口RX脚时钟 */
        USART_UX_CLK_ENABLE();                                  /* 使能串口时钟 */

        gpio_init_struct.Pin = USART_TX_GPIO_PIN;               /* 串口发送引脚号 */
        gpio_init_struct.Mode = GPIO_MODE_AF_PP;                /* 复用推挽输出 */
        gpio_init_struct.Pull = GPIO_PULLUP;                    /* 上拉 */
        gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;          /* IO速度设置为高速 */
        HAL_GPIO_Init(USART_TX_GPIO_PORT, &gpio_init_struct);

        gpio_init_struct.Pin = USART_RX_GPIO_PIN;               /* 串口RX脚 模式设置 */
        gpio_init_struct.Mode = GPIO_MODE_INPUT;
        HAL_GPIO_Init(USART_RX_GPIO_PORT, &gpio_init_struct);   /* 串口RX脚 必须设置成输入模式 */

        __HAL_LINKDMA(huart, hdmarx,dma_rx_handle);
        __HAL_LINKDMA(huart, hdmatx,dma_tx_handle);

#if USART_EN_RX
        HAL_NVIC_EnableIRQ(USART_UX_IRQn);                      /* 使能USART1中断通道 */
        /*本中断里要调FreeRTOS的API(释放信号量)，抢占优先级 >= 5*/
        HAL_NVIC_SetPriority(USART_UX_IRQn, 5, 3);              /* 组2，抢占优先级5，子优先级3 */
#endif
    }
}

//接收完成中断回调，静默超过一字符时间通知CPU处理，DMA在此刻也搬完了
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
    if (huart->Instance == USART_UX)                    
    {
        g_usart_rx_len = Size;                                              
        g_usart_rx_flag = 1;                                                

        /*释放二值信号量, 唤醒串口命令任务*/
        /* g_usart_rx_sem防御性写法，重点在于得调度器开了才能运行*/
        if (g_usart_rx_sem != NULL && xTaskGetSchedulerState() == taskSCHEDULER_RUNNING)
        {
            BaseType_t woken = pdFALSE;
            xSemaphoreGiveFromISR(g_usart_rx_sem, &woken);
            portYIELD_FROM_ISR(woken);                                  /* 有更高优先级任务被唤醒就切过去 */
        }
    }
}

//DMA发送中断不能关，关了会导致不知道什么时候发完，g_usart_tx_busy永远为1(用HAL_UART_Transmit_DMA 和 Uart_Send_Dma的时候)
//printf用的是寄存器轮询发送，跟DNA没有半毛钱关系，如果用printf的话就可以关DMA TX中断
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART_UX)
    {
        uint16_t sent = huart->TxXferSize; // 本次DMA发了几个字节？
        g_tx_tail = (g_tx_tail + sent) % TX_QUEUE_SIZE; // tail 前进了几格
        g_tx_busy = 0 ;
        if (g_tx_head != g_tx_tail) // head() != tail()，还有货！
            Uart_Start_Dma_Send_FromISR(); // 自动接着发(中断上下文, 用FromISR版本临界区)
    }
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART_UX)
    {
       HAL_UARTEx_ReceiveToIdle_DMA(&g_uart1_handle, g_rx_buffer, RXBUFFERSIZE);
    }
}

/* 从队列取一段连续数据，启动 DMA 发送(本函数不加锁, 临界区由三个外壳函数提供)
 * 任务和发送完成中断都会走到这里, 必须保证"判忙+启动"是原子的, 不原子两边会同时启动DMA */
static void Uart_Start_Dma_Send_Impl(TickType_t now)
{
    uint16_t len;

    if (g_tx_head == g_tx_tail || g_tx_busy)   /* 空的不发, 正在发也不重复启动 */
    {
        return ;
    }

    if (g_tx_head > g_tx_tail)
        len = g_tx_head - g_tx_tail; // 情况A：没绕圈
    else
        len = TX_QUEUE_SIZE - g_tx_tail; // 情况B：绕圈了，先发到最末尾，再在发送完成中断进行处理

    g_tx_busy = 1 ;
    g_tx_start_tick = now ;                     /* 记下启动时刻, 给超时兜底用 */
    HAL_UART_Transmit_DMA(&g_uart1_handle, &g_tx_queue[g_tx_tail], len);
}

/* 任务态 / 调度器未启动时调用
 * taskENTER_CRITICAL(): 走BASEPRI, 嵌套计数交给内核维护 */
static void Uart_Start_Dma_Send(void)
{
    uint32_t primask;

    if (xTaskGetSchedulerState() == taskSCHEDULER_RUNNING)
    {
        taskENTER_CRITICAL();
        Uart_Start_Dma_Send_Impl(xTaskGetTickCount());
        taskEXIT_CRITICAL();
    }
    else/*防御性判断，可以else if出来一条挂起态，这里是挂起和未启用写在了一起*/
    {
        /* 调度器还没启动: uxCriticalNesting还是初始值0xaaaaaaaa, 这时进临界区永远退不出来
         * (BASEPRI不会被清), 所以这里退回PRIMASK的保存/恢复, 同时不破坏调用前的中断状态 */
        primask = __get_PRIMASK();
        __disable_irq();
        Uart_Start_Dma_Send_Impl(0);            /* 没启动调度器时 xTaskGetTickCount() 恒为0 */
        __set_PRIMASK(primask);
    }
}

/* 中断上下文(DMA发送完成中断)调用
 * 用FromISR版的掩码保存/恢复 */
static void Uart_Start_Dma_Send_FromISR(void)
{
    UBaseType_t mask = portSET_INTERRUPT_MASK_FROM_ISR();/*PRIMASK，不依赖内核，防御性关闭中断*/

    Uart_Start_Dma_Send_Impl(xTaskGetTickCountFromISR());
    portCLEAR_INTERRUPT_MASK_FROM_ISR(mask);/*清除关闭中断标志位*/
}

/* DMA发送超时的兜底恢复
 * 只在 Uart_Send_Dma 里(任务态)调用. 如果 g_tx_busy 已经置位超过 TX_TIMEOUT_MS
 * 还没被发送完成中断清零, 就说明那个中断丢了或者DMA被意外关掉了,
 * 这时强制中止并把积压丢掉, 让串口能自己活过来 */

static void Uart_Tx_Recover_If_Timeout(void)
{
    if (g_tx_busy == 0) return;
    if ((xTaskGetTickCount() - g_tx_start_tick) <= pdMS_TO_TICKS(TX_TIMEOUT_MS)) return;
    /*这一段需要解释一下，xTaskGetTickCount g_tx_start_tick pdMS_TO_TICKS分别表示什么*/

    /* 调度器没启动时 tick 恒为0, 上面必定直接返回, 走不到这里, 所以可以放心用任务临界区 */
    taskENTER_CRITICAL();
    HAL_UART_AbortTransmit(&g_uart1_handle);   /* 停DMA通道并清标志, 否则下次Transmit_DMA会直接返回BUSY */
    g_tx_tail = g_tx_head;                     /* 积压的旧数据全丢, 免得带着脏数据继续跑 */
    g_tx_busy = 0;
    g_tx_start_tick = 0;
    taskEXIT_CRITICAL();
}

/*
 * 启动DMA发送之后会直接返回到调用的函数，并不是阻塞着等发完
 * 封装: DMA 发送字符串数据
 * 加互斥量: 多任务同时调用时整串排队, 不会你一个字节我一个字节混在一起
 * 忙则直接丢弃超出部分, 成功入队就自动启动发送
*/
void Uart_Send_Dma(const char *str)
{
    uint16_t len = strlen (str);
    uint16_t i;
    uint8_t  sched_running;
    uint32_t primask = 0;

    if (len == 0 ) return ;

    /* 调度器还没跑起来时(比如main里的上电打印)不能用互斥量, 直接发 */
    sched_running = (xTaskGetSchedulerState() == taskSCHEDULER_RUNNING);
    if (sched_running) xSemaphoreTake(g_tx_mutex, portMAX_DELAY);

    /* 入队前先看看上一次发送是不是卡死了, 卡死就先救回来 */
    Uart_Tx_Recover_If_Timeout();

    /* 入队要原子, 但临界区方式得看上下文:
     * 任务态走 taskENTER_CRITICAL(BASEPRI + 内核嵌套计数), 不再直接改PRIMASK;
     * 调度器没启动时不能进FreeRTOS临界区, 退回PRIMASK保存/恢复 */
    if (sched_running)
    {
        taskENTER_CRITICAL();
    }
    else/*防御写法*/
    {
        primask = __get_PRIMASK();
        __disable_irq();
    }

    for (i = 0 ; i < len; i++)
    {
        uint16_t next = (g_tx_head + 1 ) % TX_QUEUE_SIZE; //算下一个格子（取模=绕圈），保证头和尾之间有一个空格子隔着
        if (next == g_tx_tail) break ; //下一格撞上tail？满了就停
        g_tx_queue[g_tx_head] = str[i]; //把字节写进当前格子
        g_tx_head = next; //head 前进一格
    }

    if (sched_running) taskEXIT_CRITICAL();
    else __set_PRIMASK(primask);

    Uart_Start_Dma_Send(); /* DMA 空闲就启动(函数内部自己判忙) */

    if (sched_running) xSemaphoreGive(g_tx_mutex);
}

/*
 * 多任务安全的格式化输出
 * 为什么不用printf: fputc一个字符一锁, 两个任务的printf会交叉穿插;
 * 这里整串格式化好再交给Uart_Send_Dma(整串互斥入队), 输出就是完整一行
 */
void Uart_Printf(const char *fmt, ...)
{
    char buf[128];
    va_list ap;
    int nlen;

    va_start(ap, fmt);
    nlen = vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    if (nlen > 0) Uart_Send_Dma(buf);
}

/*
 * 任务里阻塞等待一帧串口数据(二值信号量)
 * 谁收的帧谁处理, 串口命令任务专用
 */
void Uart_Rx_WaitFrame(void)
{
    xSemaphoreTake(g_usart_rx_sem, portMAX_DELAY);
}

void USART_UX_IRQHandler(void)
{
    HAL_UART_IRQHandler(&g_uart1_handle);   /* 调用HAL库中断处理公用函数 */
}

#endif
