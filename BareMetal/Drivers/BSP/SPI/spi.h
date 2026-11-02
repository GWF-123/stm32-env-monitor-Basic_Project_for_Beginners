#ifndef __SPI_H
#define __SPI_H

#include "./SYSTEM/sys/sys.h"

#define SPIx    SPI2
#define SPIx_CLK_ENABLE() do{ __HAL_RCC_SPI2_CLK_ENABLE(); }while(0)

#define SPIx_MISO_GPIO_CLK_ENABLE() do{ __HAL_RCC_GPIOB_CLK_ENABLE(); }while(0)
#define MISO_PIN    GPIO_PIN_14
#define MISO_PORT   GPIOB

#define SPIx_MOSI_GPIO_CLK_ENABLE() do{ __HAL_RCC_GPIOB_CLK_ENABLE(); }while(0)
#define MOSI_PIN    GPIO_PIN_15
#define MOSI_PORT   GPIOB

#define SPIx_SCK_GPIO_CLK_ENABLE() do{ __HAL_RCC_GPIOB_CLK_ENABLE(); }while(0)
#define SPIx_SCK_PIN    GPIO_PIN_13
#define SPIx_SCK_PORT   GPIOB

/* SPI总线速度设置 */
#define SPI_SPEED_2         0
#define SPI_SPEED_4         1
#define SPI_SPEED_8         2
#define SPI_SPEED_16        3
#define SPI_SPEED_32        4
#define SPI_SPEED_64        5
#define SPI_SPEED_128       6
#define SPI_SPEED_256       7


void Spi2_Set_Speed(uint8_t speed);
uint8_t Spi2_Read_Write_Byte(uint8_t txdata);
void Spi2_Init(void);

#endif
