#ifndef __USART_H
#define __USART_H

#include "stdio.h"
#include "./SYSTEM/sys/sys.h"

#define USART_TX_GPIO_PORT                  GPIOA
#define USART_TX_GPIO_PIN                   GPIO_PIN_9
#define USART_TX_GPIO_CLK_ENABLE()          do{ __HAL_RCC_GPIOA_CLK_ENABLE(); }while(0)   /* PA口时钟使能 */

#define USART_RX_GPIO_PORT                  GPIOA
#define USART_RX_GPIO_PIN                   GPIO_PIN_10
#define USART_RX_GPIO_CLK_ENABLE()          do{ __HAL_RCC_GPIOA_CLK_ENABLE(); }while(0)   /* PA口时钟使能 */

#define USART_UX                            USART1
#define USART_UX_IRQn                       USART1_IRQn
#define USART_UX_IRQHandler                 USART1_IRQHandler
#define USART_UX_CLK_ENABLE()               do{ __HAL_RCC_USART1_CLK_ENABLE(); }while(0)  /* USART1 时钟使能 */

#define USART_REC_LEN               200         /* 定义最大接收字节数 200 */
#define USART_EN_RX                 1           /* 使能（1）/禁止（0）串口1接收 */

#define RXBUFFERSIZE                512
# define TX_QUEUE_SIZE   512


static void Uart_Start_Dma_Send(void);

extern UART_HandleTypeDef g_uart1_handle;       /* HAL UART句柄 */
extern uint8_t g_rx_buffer[RXBUFFERSIZE];
extern volatile uint16_t g_usart_rx_flag;       /* 接收完成标志: 1=收到一帧(主循环负责清0) */
extern volatile uint16_t g_usart_rx_len;        /* 本帧实际接收到的字节数 */

void Uart_Send_Dma(const char * str); /* 发送字符串函数 */
void Usart_Init(uint32_t bound);                /* 串口初始化函数(接口不变, main里照常调用) */

#endif
