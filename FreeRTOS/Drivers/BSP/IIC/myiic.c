#include "./BSP/IIC/myiic.h"
#include "./SYSTEM/delay/delay.h"

I2C_HandleTypeDef hi2c1;   /* I2C1 句柄 */

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
