#ifndef __APP_MAX30102_SIMPLE_H
#define __APP_MAX30102_SIMPLE_H

#include "./SYSTEM/sys/sys.h"

/* ================================================================================
 *  MAX30102 最简验证版 —— 只用来读器件ID，验证 I2C 多从机通信
 *  不修改任何基础代码，在 myiic.c 软件模拟 I2C 之上扩展
 *  和 OLED 共用同一组 SCL/SDA（PB6/PB7），通过不同 I2C 地址区分
 * ================================================================================ */

/* MAX30102 的 7 位 I2C 地址（固定，不能改）
 * 写地址 = 0x57 << 1 = 0xAE
 * 读地址 = 0x57 << 1 | 1 = 0xAF
 */
#define MAX30102_ADDR       0x57

/* PART_ID 寄存器地址 = 0xFF —— 读出来应该是 0x15（MAX30102 的固定器件ID）
 * 来源：数据手册第11页寄存器映射表，POR 列 = 0x15
 */
#define REG_PART_ID         0xFF

/* REVISION_ID 寄存器地址 = 0xFE —— 修订版本号（每个批次可能不同） */
#define REG_REVISION_ID     0xFE


/* 函数声明 */
uint8_t MAX30102_ReadID(void);   /* 读器件ID，返回读到的字节（没读到返回0） */

#endif /* __APP_MAX30102_SIMPLE_H */

