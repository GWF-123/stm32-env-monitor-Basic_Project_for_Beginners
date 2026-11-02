#include "./BSP/SPI/spi.h"

static SPI_HandleTypeDef spi_handle;

void Spi2_Init(void)
{
    SPIx_CLK_ENABLE();
    
    spi_handle.Instance = SPIx;
    spi_handle.Init.Mode = SPI_MODE_MASTER;
    spi_handle.Init.FirstBit = SPI_FIRSTBIT_MSB;
    spi_handle.Init.DataSize = SPI_DATASIZE_8BIT;
    spi_handle.Init.Direction = SPI_DIRECTION_2LINES;
    spi_handle.Init.CLKPolarity = SPI_POLARITY_HIGH;
    spi_handle.Init.CLKPhase = SPI_PHASE_2EDGE;
    spi_handle.Init.NSS = SPI_NSS_SOFT;
    spi_handle.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_256;
    spi_handle.Init.TIMode = SPI_TIMODE_DISABLE;
    spi_handle.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    spi_handle.Init.CRCPolynomial = 7;
    
    HAL_SPI_Init(&spi_handle);
    
}

void HAL_SPI_MspInit(SPI_HandleTypeDef *hspi)
{
    if(hspi ->Instance == SPIx)
    {
        GPIO_InitTypeDef gpio_init;
        
        SPIx_MISO_GPIO_CLK_ENABLE();
        SPIx_MOSI_GPIO_CLK_ENABLE();
        SPIx_SCK_GPIO_CLK_ENABLE();
        
        gpio_init.Pin = MISO_PIN;
        gpio_init.Mode = GPIO_MODE_AF_PP;
        gpio_init.Pull = GPIO_PULLUP;
        gpio_init.Speed = GPIO_SPEED_FREQ_HIGH;
        HAL_GPIO_Init(MISO_PORT, &gpio_init);
        
        gpio_init.Pin = MOSI_PIN;
        gpio_init.Mode = GPIO_MODE_AF_PP;
        HAL_GPIO_Init(MOSI_PORT, &gpio_init);
        
        gpio_init.Pin = SPIx_SCK_PIN;
        gpio_init.Mode = GPIO_MODE_AF_PP;
        HAL_GPIO_Init(SPIx_SCK_PORT, &gpio_init);
        
    }
}

/**
 * @brief       SPI2速度设置函数
 *   @note      SPI2时钟选择来自APB1, 即PCLK1, 为36Mhz
 *              SPI速度 = PCLK1 / 2^(speed + 1)
 * @param       speed   : SPI2时钟分频系数
                        取值为SPI_BAUDRATEPRESCALER_2~SPI_BAUDRATEPRESCALER_2 256
 * @retval      无
 */
void Spi2_Set_Speed(uint8_t speed)
{
    assert_param(IS_SPI_BAUDRATE_PRESCALER(speed)); /* 判断有效性 */
    __HAL_SPI_DISABLE(&spi_handle);             /* 关闭SPI */
    spi_handle.Instance->CR1 &= 0XFFC7;         /* 位3-5清零，用来设置波特率 */
    spi_handle.Instance->CR1 |= speed << 3;     /* 设置SPI速度 */
    __HAL_SPI_ENABLE(&spi_handle);              /* 使能SPI */
}

/**
 * @brief       SPI2读写一个字节数据
 * @param       txdata  : 要发送的数据(1字节)
 * @retval      接收到的数据(1字节)
 */
uint8_t Spi2_Read_Write_Byte(uint8_t txdata)
{
    uint8_t rxdata;
    HAL_SPI_TransmitReceive(&spi_handle, &txdata, &rxdata, 1, 1000);
    return rxdata; /* 返回收到的数据 */
}
