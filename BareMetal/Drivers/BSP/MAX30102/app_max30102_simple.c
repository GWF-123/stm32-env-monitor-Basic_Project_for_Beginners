#include "./MAX30102/app_max30102_simple.h"
#include "./BSP/IIC/myiic.h"    /* 软件模拟 I2C 接口 */
#include "./SYSTEM/delay/delay.h"


/* ================================================================================
 *  I2C 写一个寄存器
 *  流程：起始 → 器件地址(写) → 寄存器地址 → 数据 → 停止
 * ================================================================================ */
static void max30102_i2c_write_reg(uint8_t reg, uint8_t val)
{
    Iic_Start();
    Iic_Send_Byte(MAX30102_ADDR << 1);    /* 器件地址 + 写(0) */
    Iic_Wait_Ack();
    Iic_Send_Byte(reg);                    /* 寄存器地址 */
    Iic_Wait_Ack();
    Iic_Send_Byte(val);                    /* 要写的数据 */
    Iic_Wait_Ack();
    Iic_Stop();
}


/* ================================================================================
 *  I2C 读一个寄存器
 *  流程：起始 → 器件地址(写) → 寄存器地址 → 重复起始 → 器件地址(读) → 读1字节(NACK) → 停止
 * ================================================================================ */
static uint8_t max30102_i2c_read_reg(uint8_t reg)
{
    uint8_t data;

    Iic_Start();
    Iic_Send_Byte(MAX30102_ADDR << 1);    /* 器件地址 + 写(0) —— 告诉从机要读哪个寄存器 */
    if (Iic_Wait_Ack() != 0)               /* 没应答 = 器件不存在 */
    {
        Iic_Stop();
        return 0;
    }
    Iic_Send_Byte(reg);                    /* 寄存器地址 */
    Iic_Wait_Ack();

    Iic_Start();                           /* 重复起始 */
    Iic_Send_Byte((MAX30102_ADDR << 1) | 0x01);  /* 器件地址 + 读(1) */
    if (Iic_Wait_Ack() != 0)
    {
        Iic_Stop();
        return 0;
    }
    data = Iic_Read_Byte(0);               /* 读1字节，最后一个字节发 NACK */
    Iic_Stop();

    return data;
}


/* ================================================================================
 *  读 MAX30102 的器件 ID（PART_ID）
 *  返回值：读到的 ID 字节（正常应为 0x15），读不到返回 0
 * ================================================================================ */
uint8_t MAX30102_ReadID(void)
{
    uint8_t id;

    /* 先尝试读 PART_ID 寄存器（0xFE） */
    id = max30102_i2c_read_reg(REG_PART_ID);

    /* 如果读到 0x15，说明是真的 MAX30102 */
    return id;
}

