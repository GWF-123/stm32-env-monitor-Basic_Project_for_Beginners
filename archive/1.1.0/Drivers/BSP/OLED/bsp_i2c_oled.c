#include "./BSP/OLED/bsp_i2c_oled.h"
#include "./BSP/IIC/myiic.h"
#include "./SYSTEM/usart/usart.h"
#include "./BSP/FONTS/fonts.h"
#include <string.h>

/**
  * @brief  检测I2C设备是否存在（软件模拟I2C版本）
  * @param  slave_addr：7位I2C设备地址
  * @retval SUCCESS 表示设备存在，ERROR 表示设备不存在
  */
ErrorStatus OLED_CheckDevice(uint8_t slave_addr)
{
    /* 发起始信号 */
    Iic_Start();
    
    /* 发送从机地址 + 写位(0) */
    Iic_Send_Byte(slave_addr << 1);
    
    /* 等待应答 */
    if (Iic_Wait_Ack() == 0)   /* 0 = 收到应答 */
    {
        Iic_Stop();
        return SUCCESS;
    }
    
    Iic_Stop();
    return ERROR;
}

/**
  * @brief  写入单字节数据到 OLED（软件模拟I2C版本）
  * @param  cmd: 写入模式，OLED_WR_CMD(0x00)=写命令，OLED_WR_DATA(0x40)=写数据
  * @param  byte: 需要写入的具体数据或命令字节
  * @retval SUCCESS 表示写入成功，ERROR 表示写入失败
  */
ErrorStatus OLED_WriteByte(uint8_t cmd, uint8_t byte)
{
    /* 1. 起始信号 */
    Iic_Start();
    
    /* 2. 发送从机地址 + 写位 */
    Iic_Send_Byte(OLED_SLAVER_ADDR << 1);
    if (Iic_Wait_Ack() != 0)   /* 等待应答，非0表示失败 */
    {
        Iic_Stop();
        return ERROR;
    }
    
    /* 3. 发送控制字节（命令/数据标识） */
    Iic_Send_Byte(cmd);
    if (Iic_Wait_Ack() != 0)
    {
        Iic_Stop();
        return ERROR;
    }
    
    /* 4. 发送数据字节 */
    Iic_Send_Byte(byte);
    if (Iic_Wait_Ack() != 0)
    {
        Iic_Stop();
        return ERROR;
    }
    
    /* 5. 停止信号 */
    Iic_Stop();
    
    return SUCCESS;
}

/**
  * @brief  向 OLED 写入多个字节（软件模拟I2C版本）
  * @param  cmd: 写入模式，OLED_WR_CMD(0x00)=写命令，OLED_WR_DATA(0x40)=写数据
  * @param  buffer: 指向需要写入的数据缓冲区指针
  * @param  num: 要写入的数据字节数
  * @retval SUCCESS 表示写入成功，ERROR 表示写入失败
  */
ErrorStatus OLED_WriteBuffer(uint8_t cmd, uint8_t *buffer, uint32_t num)
{
    if (buffer == 0 || num == 0)
        return ERROR;
    
    /* 1. 起始信号 */
    Iic_Start();
    
    /* 2. 发送从机地址 + 写位 */
    Iic_Send_Byte(OLED_SLAVER_ADDR << 1);
    if (Iic_Wait_Ack() != 0)
    {
        Iic_Stop();
        return ERROR;
    }
    
    /* 3. 发送控制字节（命令/数据标识） */
    Iic_Send_Byte(cmd);
    if (Iic_Wait_Ack() != 0)
    {
        Iic_Stop();
        return ERROR;
    }
    
    /* 4. 连续发送数据字节 */
    for (uint32_t i = 0; i < num; i++)
    {
        Iic_Send_Byte(buffer[i]);
        if (Iic_Wait_Ack() != 0)
        {
            Iic_Stop();
            return ERROR;
        }
    }
    
    /* 5. 停止信号 */
    Iic_Stop();
    
    return SUCCESS;
}

/**
  * @brief  设置 OLED 显示的光标位置
  * @param  y：页地址，范围 0~7，对应 OLED 的行（每页8像素高）
  * @param  x：列地址，范围 0~127，对应 OLED 的列位置
  * @retval 无
  */
void OLED_SetPos(uint8_t y, uint8_t x)
{
    uint8_t pos_buf[3] = {
        0xB0 + y,               	/* 设置页地址命令，0xB0 是页地址起始，后加页码 */
        ((x & 0xF0) >> 4) | 0x10,  /* 设置列地址高4位 + 0x10 */
        (x & 0x0F)              	/* 设置列地址低4位 */
    };

    OLED_WriteBuffer(OLED_WR_CMD, pos_buf, sizeof(pos_buf));
}

/**
  * @brief  填充整个屏幕
  * @param  fill_data: 要填充的数据（0x00 全黑，0xFF 全亮等）
  * @retval 无
  */
void OLED_Fill(uint8_t fill_data)
{
    uint8_t data_buffer_temp[128];
    memset(data_buffer_temp, fill_data, 128);

    /* OLED共有8页，每页128列，循环逐页写入数据 */
    for(uint8_t m = 0; m < 8; m++)
    {
        OLED_SetPos(m, 0);//第m页的开头开始去写东西，详细见笔记
        //因为没有自动翻页所以的得去循环，如果数组为1的话则表示点亮八行1列的像素点
        OLED_WriteBuffer(OLED_WR_DATA, data_buffer_temp, OLED_ARRAY_SIZE(data_buffer_temp));
    }
}

/**
  * @brief  清屏函数
  * @param  无
  * @retval  无
  */
void OLED_CLE(void)
{
    OLED_Fill(0x00);  /* 填充 0x00，所有像素点灭 */
}

/**
  * @brief  OLED全屏点亮函数
  * @param  无
  * @retval  无
  */
void OLED_FillFull(void)
{
    OLED_Fill(0xFF);  /* 填充 0xFF，所有像素点亮 */
}

/**
  * @brief  OLED 初始化
  * @param  无
  * @retval  无
  */
void OLED_Init(void)
{
    HAL_Delay(100);  /* 延时保证电源稳定 */

    /* 检测OLED设备（死等，确保设备连接正常） */
    while (OLED_CheckDevice(OLED_SLAVER_ADDR) != SUCCESS);

    /* 控制显示 */
    OLED_WriteByte(OLED_WR_CMD,0xAE);  /* 显示关闭 */

    /* 控制内存寻址模式 */
    OLED_WriteByte(OLED_WR_CMD,0x20);  /* 设置内存寻址模式 */
    OLED_WriteByte(OLED_WR_CMD,0x02);  /* 页面寻址模式 */

    /* 页起始地址 */
    OLED_WriteByte(OLED_WR_CMD,0xB0);  /* 页面起始地址 */

    /* COM输出扫描方向 */
    OLED_WriteByte(OLED_WR_CMD,0xA1);  /* 左右方向 */
    OLED_WriteByte(OLED_WR_CMD,0xC8);  /* 上下方向 */

    /* 页内列起始地址 */
    OLED_WriteByte(OLED_WR_CMD,0x00);  /* 列地址低位 */
    OLED_WriteByte(OLED_WR_CMD,0x10);  /* 列地址高位 */

    /* 页内行起始地址 */
    OLED_WriteByte(OLED_WR_CMD,0x40);  /* 起始行地址 */

    /* 设置对比度 */
    OLED_WriteByte(OLED_WR_CMD,0x81);  /* 对比度控制 */
    OLED_WriteByte(OLED_WR_CMD,0xff);

    /* 设置显示方向 */
    OLED_WriteByte(OLED_WR_CMD,0xA1);  /* SEG映射 */
    OLED_WriteByte(OLED_WR_CMD,0xA6);  /* 正常显示 */

    /* 设置多路复用率 */
    OLED_WriteByte(OLED_WR_CMD,0xA8);
    OLED_WriteByte(OLED_WR_CMD,0x3F);

    /* 全屏显示 */
    OLED_WriteByte(OLED_WR_CMD,0xA4);  /* 恢复RAM内容显示 */

    /* 设置显示偏移量 */
    OLED_WriteByte(OLED_WR_CMD,0xd3);
    OLED_WriteByte(OLED_WR_CMD,0x00);

    /* 设置显示时钟分频比/振荡器频率 */
    OLED_WriteByte(OLED_WR_CMD,0xD5);
    OLED_WriteByte(OLED_WR_CMD,0xf0);

    /* 设置预充期 */
    OLED_WriteByte(OLED_WR_CMD,0xD9);
    OLED_WriteByte(OLED_WR_CMD,0x22);

    /* 设置com引脚硬件配置 */
    OLED_WriteByte(OLED_WR_CMD,0xDA);
    OLED_WriteByte(OLED_WR_CMD,0x12);

    /* 设置VCOMH取消选择级别 */
    OLED_WriteByte(OLED_WR_CMD,0xDB);
    OLED_WriteByte(OLED_WR_CMD,0x20);

    OLED_WriteByte(OLED_WR_CMD,0x8D);  /* DC-DC使能 */
    OLED_WriteByte(OLED_WR_CMD,0x14);

    /* 显示打开 */
    OLED_WriteByte(OLED_WR_CMD,0xAF);  /* 显示开启 */

    OLED_CLE();      /* 清屏 */

    //OLED_FillFull(); /* 全屏点亮（白屏） */
}

/**
  * @brief  显示16x16点阵的汉字
  * @param  y: 页地址（0~7），OLED以8像素为一页
  * @param  x: 列地址（0~127）
  * @param  n: 汉字索引号，在汉字库中的序号
  * @param  data_cn: 指向汉字点阵数据的指针（每个汉字32字节）
  * @retval 无
  */

void OLED_ShowChinese(uint8_t y, uint8_t x, uint8_t n, const uint8_t data_cn[][16])
{
    OLED_SetPos(y, x);  // 设置到第一页位置
    for (uint8_t i = 0; i < 16; i++)  // 写入上半部分 16 字节
    {
        OLED_WriteByte(OLED_WR_DATA, data_cn[n * 2][i]);
    }

    OLED_SetPos(y + 1, x);  // 设置到第二页位置
    for (uint8_t i = 0; i < 16; i++)  // 写入下半部分 16 字节
    {
        OLED_WriteByte(OLED_WR_DATA, data_cn[n * 2 + 1][i]);
    }
}

/**
  * @brief  显示16x16点阵的汉字，接口参数为行/列索引
  * @param  line: 0~3，纵向文本行数（每行16像素高，2页）
  * @param  offset: 0~7，横向汉字偏移（每个汉字16像素宽）
  * @param  n: 汉字索引号
  * @retval 无
  */
void OLED_ShowChinese_F16X16(uint8_t line, uint8_t offset, uint8_t n)
{
    OLED_ShowChinese(line * 2, offset * 16, n, chinese_library_16x16);
}

/**
  * @brief  OLED 显示单个 ASCII 字符
  * @param  y: OLED 页地址（0~7），每页高度为8像素
  * @param  x: OLED 列地址（0~127），字符显示的起始列
  * @param  char_data: 要显示的 ASCII 字符，范围 32~127（可见字符）
  * @param  textsize: 字体大小，支持两种尺寸：
  *                   TEXTSIZE_F6X8 （6x8 点阵）
  *                   TEXTSIZE_F8X16（8x16 点阵）
  * @retval 无
  *
  * @note   字符点阵存储在 ascll_code_6x8 和 ascll_code_8x16 两个数组中，
  *         通过 char_data - 32 计算对应字符的索引位置。
  *         6x8 字体一行显示即可，8x16 字体需要占用两页显示（上下两部分）。
  */
void OLED_ShowChar(uint8_t y, uint8_t x, uint8_t char_data, uint8_t textsize)
{
    uint32_t addr = char_data - 32;  // 计算字符在点阵数组中的索引

    switch(textsize)
    {
        case TEXTSIZE_F6X8:   // 6x8点阵字体，宽6像素，高8像素
            OLED_SetPos(y, x);  // 设置光标位置
            for(uint8_t i = 0; i < 6; i++)  // 依次写入6字节点阵数据
            {
                OLED_WriteByte(OLED_WR_DATA, (uint8_t)ascii_code_6x8[addr][i]);
            }
            break;

        case TEXTSIZE_F8X16:  // 8x16点阵字体，宽8像素，高16像素
            OLED_SetPos(y, x);  // 设置光标位置显示上半部分
            for(uint8_t i = 0; i < 8; i++)  // 写入8字节点阵数据（上半部分）
            {
                OLED_WriteByte(OLED_WR_DATA, (uint8_t)ascii_code_8x16[addr][i]);
            }
            OLED_SetPos(y + 1, x);  // 设置下一页光标显示下半部分
            for(uint8_t i = 0; i < 8; i++)  // 写入剩余8字节点阵数据（下半部分）
            {
                OLED_WriteByte(OLED_WR_DATA, (uint8_t)ascii_code_8x16[addr][i + 8]);
            }
            break;

        default:
            break;
    }
}

/**
  * @brief  显示字符串
  * @param  y: OLED页地址（0~7）
  * @param  x: OLED列地址（0~127）
  * @param  string_data: 指向字符串的指针（以'\0'结束）
  * @param  textsize: 字体大小（6x8或8x16）
  * @retval 无
  */

void OLED_ShowString(uint8_t y, uint8_t x, uint8_t *string_data, uint8_t textsize)
{
    for(uint8_t i = 0; *string_data != '\0'; i++)
    {
            // 逐字符调用 OLED_ShowChar，x坐标根据字符宽度递增，实现横向连续显示
        OLED_ShowChar(y, x + i * textsize, *string_data++, textsize);
    }
}

/**
  * @brief  显示字符串，8x16字体版
  * @param  line: 行号（0~3），每行16像素高（2页）
  * @param  offset: 列偏移（0~15）
  * @param  string_data: 指向字符串的指针
  * @retval 无
  */

void OLED_ShowString_F8X16(uint8_t line, uint8_t offset, uint8_t *string_data)
{
    OLED_ShowString(line * 2, offset * 8, string_data, TEXTSIZE_F8X16);
}


