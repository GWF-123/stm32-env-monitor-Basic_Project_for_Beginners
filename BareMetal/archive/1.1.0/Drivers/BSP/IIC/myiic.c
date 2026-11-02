#include "./BSP/IIC/myiic.h"
#include "./SYSTEM/delay/delay.h"


/******************************************************************************************/
/* 硬件I2C 句柄定义 —— 原 bsp_i2c.c 中的内容 */

I2C_HandleTypeDef hi2c1;   /* I2C1 句柄 */


/******************************************************************************************/
/* 软件模拟I2C 部分 —— 原 myiic.c 的内容 */

/**
 * @brief       IIC延时函数,用于控制IIC读写速度
 * @param       无
 * @retval      无
 */
static void Iic_Delay(void)
{
    delay_us(2);    /* 2us的延时, 读写速度在250Khz以内 */
}

/**
 * @brief       初始化软件IIC（GPIO模拟）
 * @param       无
 * @retval      无
 * @note
软件I2C和硬件I2C不能同时使用，二选一
 */
void Iic_Init(void)
{
    GPIO_InitTypeDef gpio_init_struct;

    IIC_SCL_GPIO_CLK_ENABLE();  /* SCL引脚时钟使能 */
    IIC_SDA_GPIO_CLK_ENABLE();  /* SDA引脚时钟使能 */

    gpio_init_struct.Pin = IIC_SCL_GPIO_PIN;
    gpio_init_struct.Mode = GPIO_MODE_OUTPUT_PP;        /* 推挽输出 */
    gpio_init_struct.Pull = GPIO_PULLUP;                /* 上拉 */
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;      /* 高速 */
    HAL_GPIO_Init(IIC_SCL_GPIO_PORT, &gpio_init_struct);/* SCL */

    gpio_init_struct.Pin = IIC_SDA_GPIO_PIN;
    gpio_init_struct.Mode = GPIO_MODE_OUTPUT_OD;        /* 开漏输出 */
    HAL_GPIO_Init(IIC_SDA_GPIO_PORT, &gpio_init_struct);/* SDA */
    /* SDA引脚模式设置,开漏输出,上拉, 这样就不用再设置IO方向了, 开漏输出的时候(=1), 也可以读取外部信号的高低电平 */

    Iic_Stop();     /* 停止总线上所有设备 */
}

/**
 * @brief       产生IIC起始信号
 * @param       无
 * @retval      无
 */

void Iic_Start(void)
{
    IIC_SDA(1);
    IIC_SCL(1);
    Iic_Delay();
    IIC_SDA(0);     /* START信号: 当SCL为高时, SDA从高变成低, 表示起始信号 */
    Iic_Delay();
    IIC_SCL(0);     /* 钳住I2C总线，准备发送或接收数据 */
    Iic_Delay();
}

/**
 * @brief       产生IIC停止信号
 * @param       无
 * @retval      无
 */

void Iic_Stop(void)
{
    IIC_SDA(0);     /* STOP信号: 当SCL为高时, SDA从低变成高, 表示停止信号 */
    Iic_Delay();
    IIC_SCL(1);
    Iic_Delay();
    IIC_SDA(1);     /* 发送I2C总线结束信号 */
    Iic_Delay();
}

/**
 * @brief       等待应答信号到来
 * @param       无
 * @retval      1，接收应答失败
 *              0，接收应答成功
 */
uint8_t Iic_Wait_Ack(void)
{
    uint8_t waittime = 0;
    uint8_t rack = 0;

    IIC_SDA(1);     /* 主机释放SDA线(此时外部器件可以拉低SDA线) */
    Iic_Delay();
    IIC_SCL(1);     /* SCL=1, 此时从机可以返回ACK */
    Iic_Delay();

    while (IIC_READ_SDA)    /* 等待应答 */
    {
        waittime++;

        if (waittime > 250)
        {
            Iic_Stop();
            rack = 1;
            break;
        }
    }

    IIC_SCL(0);     /* SCL=0, 结束ACK检查 */
    Iic_Delay();
    return rack;
}

/**
 * @brief       产生ACK应答
 * @param       无
 * @retval      无
 */
void Iic_Ack(void)
{
    IIC_SDA(0);     /* SCL 0 -> 1  时 SDA = 0,表示应答 */
    Iic_Delay();
    IIC_SCL(1);     /* 产生一个时钟 */
    Iic_Delay();
    IIC_SCL(0);
    Iic_Delay();//下面可以不用管
    IIC_SDA(1);     /* 主机释放SDA线 */
    Iic_Delay();
}

/**
 * @brief       不产生ACK应答
 * @param       无
 * @retval      无
 */
void Iic_Nack(void)
{
    IIC_SDA(1);     /* SCL 0 -> 1  时 SDA = 1,表示不应答 */
    Iic_Delay();
    IIC_SCL(1);     /* 产生一个时钟 */
    Iic_Delay();
    IIC_SCL(0);
    Iic_Delay();
}

/**
 * @brief       IIC发送一个字节
 * @param       data: 要发送的数据
 * @retval      无
 */
void Iic_Send_Byte(uint8_t data)
{
    uint8_t t;
    
    for (t = 0; t < 8; t++)
    {
        IIC_SDA((data & 0x80) >> 7);    /* 高位先发送 */
        Iic_Delay();
        IIC_SCL(1);
        Iic_Delay();
        IIC_SCL(0);
        data <<= 1;     /* 左移1位,用于下一次发送 */
    }
    IIC_SDA(1);         /* 发送完成, 主机释放SDA线 */
}

/**
 * @brief       IIC读取一个字节
 * @param       ack:  ack=1时，发送ack; ack=0时，发送nack
 * @retval      接收到的数据
 */
uint8_t Iic_Read_Byte(uint8_t ack)
{
    uint8_t i, receive = 0;

    for (i = 0; i < 8; i++ )    /* 接收1个字节数据 */
    {
        receive <<= 1;  /* 高位先输出,所以先收到的数据位要左移 */
        IIC_SCL(1);
        Iic_Delay();

        if (IIC_READ_SDA)
        {
            receive++;
        }
        
        IIC_SCL(0);
        Iic_Delay();
    }

    if (!ack)
    {
        Iic_Nack();     /* 发送nACK */
    }
    else
    {
        Iic_Ack();      /* 发送ACK */
    }

    return receive;
}


/******************************************************************************************/
/* 硬件I2C 部分 —— 原 bsp_i2c.c 的内容 */

/**
  * @brief  初始化硬件 I2C1
  * @note   I2C1 的引脚配置：SDA -> PB7，SCL -> PB6
  *         软件I2C和硬件I2C不能同时使用，二选一
  */
void MX_I2C1_Init(void)
{
    hi2c1.Instance = I2C1;                      /* 选择使用的I2C外设为I2C1 */
    hi2c1.Init.ClockSpeed = 400000;            /* 设置I2C时钟频率为400kHz，属于高速模式 */
    hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;   /* 设置时钟占空比为2（标准模式常用） */
    hi2c1.Init.OwnAddress1 = 0;                /* 主机模式下自定义地址，0表示不使用 */
    hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;  /* 使用7位地址模式 */
    hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE; /* 禁用双地址模式 */
    hi2c1.Init.OwnAddress2 = 0;                /* 第二地址无效 */
    hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE; /* 禁用通用呼叫模式 */
    hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;     /* 允许时钟拉伸 */

    /* 调用HAL库函数初始化I2C1，若失败则调用错误处理函数 */
//    if (HAL_I2C_Init(&hi2c1) != HAL_OK)
//    {
//        Error_Handler();
//    }
}

/**
  * @brief  I2C外设相关GPIO和时钟初始化，HAL库自动调用
  * @param  i2cHandle 指向I2C句柄的指针
  */
void HAL_I2C_MspInit(I2C_HandleTypeDef* i2cHandle)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* 判断是否为I2C1外设 */
    if (i2cHandle->Instance == I2C1)
    {
        /* 开启GPIOB端口时钟 */
        __HAL_RCC_GPIOB_CLK_ENABLE();

        /** 配置I2C1的SCL和SDA引脚
          PB6 -> I2C1_SCL
          PB7 -> I2C1_SDA
        */
        GPIO_InitStruct.Pin = GPIO_PIN_6 | GPIO_PIN_7;     /* 选择PB6和PB7 */
        GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;              /* 复用开漏模式（I2C必选） */
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;        /* 设置引脚高速模式 */
        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);              /* 初始化GPIOB对应引脚 */

        /* 使能I2C1时钟 */
        __HAL_RCC_I2C1_CLK_ENABLE();
    }
}

/**
  * @brief  I2C外设GPIO和时钟反初始化，HAL库自动调用
  * @param  i2cHandle 指向I2C句柄的指针
  */
void HAL_I2C_MspDeInit(I2C_HandleTypeDef* i2cHandle)
{
    /* 判断是否为I2C1外设 */
    if (i2cHandle->Instance == I2C1)
    {
        /* 关闭I2C1时钟 */
        __HAL_RCC_I2C1_CLK_DISABLE();

        /** 反初始化I2C1对应的GPIO引脚 */
        HAL_GPIO_DeInit(GPIOB, GPIO_PIN_6);   /* 释放PB6引脚 */
        HAL_GPIO_DeInit(GPIOB, GPIO_PIN_7);   /* 释放PB7引脚 */
    }
}
