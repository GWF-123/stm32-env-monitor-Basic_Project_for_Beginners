#include "./SYSTEM/sys/sys.h"
#include "./SYSTEM/usart/usart.h"
#include "./BSP/DMA/dma.h"
#include <string.h>                                     /* ==========【新增】memcpy用 ========== */

/* 如果使用os,则包括下面的头文件即可. */
#if SYS_SUPPORT_OS
#include "os.h" /* os 使用 */
#endif

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
 * 寄存器级操作, 比HAL_UART_Transmit更直接, 任何HAL版本都能用 */
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


uint8_t g_rx_buffer[RXBUFFERSIZE];          /* HAL库底层接收缓冲(一帧数据先落在这里) */

UART_HandleTypeDef g_uart1_handle;          /* UART句柄 */

void Usart_Init(uint32_t baudrate)
{
    
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
        HAL_NVIC_SetPriority(USART_UX_IRQn, 2, 3);              /* 组2，最低优先级:抢占优先级2，子优先级3 */
#endif
    }
}

//接收完成中断回调，静默超过一字符时间通知CPU处理，DMA在此刻也搬完了
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
    if (huart->Instance == USART_UX)                    /* 如果是串口1 */
    {
        g_usart_rx_len = Size;                                              /* 记录长度 */
        g_usart_rx_flag = 1;                                                /* 通知主循环 */
        
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
            Uart_Start_Dma_Send(); // 自动接着发
    }
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART_UX)
    {
       HAL_UARTEx_ReceiveToIdle_DMA(&g_uart1_handle, g_rx_buffer, RXBUFFERSIZE);
    }
}

/* 从队列取一段连续数据，启动 DMA 发送 */ 
static void Uart_Start_Dma_Send(void) 
{
    uint16_t len; 
    if (g_tx_head == g_tx_tail) return ; // 空就退出


    if (g_tx_head > g_tx_tail)
        len = g_tx_head - g_tx_tail; // 情况A：没绕圈
    else
        len = TX_QUEUE_SIZE - g_tx_tail; // 情况B：绕圈了，先发到最末尾，再在发送完成中断进行处理
    
    g_tx_busy = 1 ;
    HAL_UART_Transmit_DMA(&g_uart1_handle, &g_tx_queue[g_tx_tail], len);
}



/*
 * 启动DMA发送之后会直接返回到调用的函数，并不是阻塞着等发完
 * 封装: DMA 发送字符串数据
 * 忙则直接返回 0, 成功启动返回 1
*/
void Uart_Send_Dma(const char *str)
{
    uint16_t len = strlen (str); 
    uint16_t i; 
    
    if (len == 0 ) return ;

    __disable_irq(); /* 关中断保护队列 在core_m3.h中*/ 
    
    for (i = 0 ; i < len; i++)
    { 
        uint16_t next = (g_tx_head + 1 ) % TX_QUEUE_SIZE; //算下一个格子（取模=绕圈），保证头和尾之间有一个空格子隔着
        if (next == g_tx_tail) break ; //下一格撞上tail？满了就停
        g_tx_queue[g_tx_head] = str[i]; //把字节写进当前格子
        g_tx_head = next; //head 前进一格
    }

    __enable_irq(); 
    
    if (!g_tx_busy)/* DMA 空闲就启动 */ 
        Uart_Start_Dma_Send();
}



void USART_UX_IRQHandler(void)
{
    
#if SYS_SUPPORT_OS                          /* 使用OS */
    OSIntEnter();
#endif

    HAL_UART_IRQHandler(&g_uart1_handle);   /* 调用HAL库中断处理公用函数 */

#if SYS_SUPPORT_OS                          /* 使用OS */
    OSIntExit();
#endif

}

#endif
