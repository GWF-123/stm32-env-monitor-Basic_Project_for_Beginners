#ifndef __MYIIC_H
#define __MYIIC_H

#include "./SYSTEM/sys/sys.h"

/******************************************************************************************/
/* 引脚 定义 —— 软件I2C 和 硬件I2C 共用 PB6(SCL) / PB7(SDA) */

#define IIC_SCL_GPIO_PORT               GPIOB
#define IIC_SCL_GPIO_PIN                GPIO_PIN_6
#define IIC_SCL_GPIO_CLK_ENABLE()       do{ __HAL_RCC_GPIOB_CLK_ENABLE(); }while(0)

#define IIC_SDA_GPIO_PORT               GPIOB
#define IIC_SDA_GPIO_PIN                GPIO_PIN_7
#define IIC_SDA_GPIO_CLK_ENABLE()       do{ __HAL_RCC_GPIOB_CLK_ENABLE(); }while(0)

/******************************************************************************************/
/* 软件模拟I2C —— IO操作宏 */

#define IIC_SCL(x)        do{ x ? \
                              HAL_GPIO_WritePin(IIC_SCL_GPIO_PORT, IIC_SCL_GPIO_PIN, GPIO_PIN_SET) : \
                              HAL_GPIO_WritePin(IIC_SCL_GPIO_PORT, IIC_SCL_GPIO_PIN, GPIO_PIN_RESET); \
                          }while(0)

#define IIC_SDA(x)        do{ x ? \
                              HAL_GPIO_WritePin(IIC_SDA_GPIO_PORT, IIC_SDA_GPIO_PIN, GPIO_PIN_SET) : \
                              HAL_GPIO_WritePin(IIC_SDA_GPIO_PORT, IIC_SDA_GPIO_PIN, GPIO_PIN_RESET); \
                          }while(0)

#define IIC_READ_SDA     HAL_GPIO_ReadPin(IIC_SDA_GPIO_PORT, IIC_SDA_GPIO_PIN)

/******************************************************************************************/
/* 硬件I2C —— 句柄声明 */

extern I2C_HandleTypeDef hi2c1;   /* I2C1 句柄，定义在 myiic.c 中 */

/******************************************************************************************/
/* 函数声明 */

/* ===== 软件模拟I2C 函数 ===== */
void Iic_Init(void);            /* 初始化软件I2C的IO口 */
void Iic_Start(void);           /* 发送IIC开始信号 */
void Iic_Stop(void);            /* 发送IIC停止信号 */
void Iic_Ack(void);             /* IIC发送ACK信号 */
void Iic_Nack(void);            /* IIC不发送ACK信号 */
uint8_t Iic_Wait_Ack(void);     /* IIC等待ACK信号 */
void Iic_Send_Byte(uint8_t txd);/* IIC发送一个字节 */
uint8_t Iic_Read_Byte(unsigned char ack);/* IIC读取一个字节 */

/* ===== 硬件I2C 函数 ===== */
void MX_I2C1_Init(void);        /* 硬件I2C1初始化 */

#endif
