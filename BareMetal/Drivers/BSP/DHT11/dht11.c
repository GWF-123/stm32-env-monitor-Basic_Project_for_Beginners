#include "./BSP/DHT11/dht11.h"
#include "./SYSTEM/delay/delay.h"

/**
 * @brief       复位DHT11，手册的步骤二
 * @param       data: 要写入的数据
 * @retval      无
 */
static void Dht11_Reset(void)
{
    DHT11_DQ_OUT(0);    /* 拉低DQ */
    delay_ms(20);       /* 拉低至少18ms */
    DHT11_DQ_OUT(1);    /* DQ=1 */
    delay_us(30);       /* 主机拉高10~35us */
}

/**
 * @brief       等待DHT11的回应，步骤三
 * @param       无
 * @retval      0, DHT11正常
 *              1, DHT11异常/不存在
 */
uint8_t Dht11_Check(void)
{
    uint8_t retry = 0;
    uint8_t rval = 0;

    while (DHT11_DQ_IN && retry < 100)  /* DHT11会拉低78~88us，超了表示从机没响应*/
    {
        retry++;
        delay_us(1);
    }

    if (retry >= 100)
    {
        rval = 1;
    }
    else
    {
        retry = 0;
/* 拉低后会再次拉高80~92us，等待低电平78~88us结束，超了就是从机没响应 */
        while (!DHT11_DQ_IN && retry < 100) 
        {
            retry++;
            delay_us(1);
        }
        if (retry >= 100) rval = 1;
    }
    
    return rval;
}

/**
 * @brief       从DHT11读取一个位，步骤四
 * @param       无
 * @retval      读取到的位值: 0 / 1
 */
uint8_t Dht11_Read_Bit(void)
{
    uint8_t retry = 0;

    while (DHT11_DQ_IN && retry < 100)  /* 刚开始采集数据是高电平，等待变为低电平*/
    {
        retry++;
        delay_us(1);
    }

    retry = 0;

    while (!DHT11_DQ_IN && retry < 100) /* 低电平等待变高电平 */
    {
        retry++;
        delay_us(1);
    }

    delay_us(40);       /* 等待40us 延时完看看是高还是低，位数据"0"格式已经为低电平了*/

    if (DHT11_DQ_IN)    /* 根据引脚状态返回 bit*/
    {
        return 1;
    }
    else 
    {
        return 0;
    }
}

/**
 * @brief       从DHT11读取一个字节
 * @param       无
 * @retval      读到的数据
 */
static uint8_t Dht11_Read_Byte(void)
{
    uint8_t i, data = 0;

    for (i = 0; i < 8; i++)         /* 循环读取8位数据 */
    {
        data <<= 1;                 /* 高位数据先输出, 先左移一位 */
        data |= Dht11_Read_Bit();   /* 读取1bit数据 */
    }

    return data;
}

/**
 * @brief       从DHT11读取一次数据
 * @param       temp: 温度值(范围:-20~60℃)
 * @param       humi: 湿度值(范围:5-95%RH)
 * @retval      0, 正常.
 *              1, 失败
 */
uint8_t Dht11_Read_Data(uint8_t *temp, uint8_t *humi)
{
    uint8_t buf[5];
    uint8_t i;
    
    Dht11_Reset();

    if (Dht11_Check() == 0)
    {
        for (i = 0; i < 5; i++)     /* 读取40位数据 */
        {
            buf[i] = Dht11_Read_Byte();
        }

        if ((buf[0] + buf[1] + buf[2] + buf[3]) == buf[4])
        {
            *humi = buf[0];
            *temp = buf[2];
        }
    }
    else
    {
        return 1;
    }
    
    return 0;
}

/**
 * @brief       初始化DHT11的IO口 DQ 同时检测DHT11的存在
 * @param       无
 * @retval      0, 正常
 *              1, 不存在/不正常
 */
uint8_t Dht11_Init(void)
{
    GPIO_InitTypeDef gpio_init_struct;

    DHT11_DQ_GPIO_CLK_ENABLE();     /* 开启DQ引脚时钟 */

    gpio_init_struct.Pin = DHT11_DQ_GPIO_PIN;
    gpio_init_struct.Mode = GPIO_MODE_OUTPUT_OD;            //选择开漏输出，因为开漏输出释放总线可以让从机接取
    gpio_init_struct.Pull = GPIO_PULLUP;                    /* 上拉 */
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;          /* 高速 */
    HAL_GPIO_Init(DHT11_DQ_GPIO_PORT, &gpio_init_struct);   /* 初始化DHT11_DQ引脚 */
    

    Dht11_Reset();
    return Dht11_Check();
}
























