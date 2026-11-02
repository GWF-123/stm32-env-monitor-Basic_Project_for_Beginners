#ifndef __APP_SAVE_H
#define __APP_SAVE_H

#include "./SYSTEM/sys/sys.h"


/* =========================================================
 *  掉电保存 — 配置参数
 * ========================================================= */

/* 保存在 W25Q128 的哪个位置 —— 第 0 个扇区起始地址 */
#define SAVE_ADDR_START     0x00000000

/* 魔术字：用来判断 Flash 里的数据是不是有效的保存数据
 * 第一次上电时 Flash 里全是 0xFF，读到不是 SAVE_MAGIC 就用默认值 */
#define SAVE_MAGIC          0x5356      /* 'S' 'V' = Save */

/* 预留字节数：以后新增保存项时用，不用改整体结构 */
#define SAVE_RESERVED_NUM   4


/* =========================================================
 *  保存的数据结构
 *  总共 16 字节（方便以后对齐和扩展）
 *  字节  0~1 : magic    (2字节, 固定 0x5356)
 *  字节  2~3 : light_val (2字节, 光敏ADC值)
 *  字节  4~5 : pwm_val   (2字节, PWM占空比)
 *  字节  6   : page_num  (1字节, 当前页码)
 *  字节  7~10: reserved  (4字节, 预留)
 *  字节 11   : checksum  (1字节, 校验和)
 *  字节 12~15: padding   (4字节, 凑16字节对齐, 填0xFF)
 * ========================================================= */
typedef struct
{
    uint16_t light_val;                 /* 光敏 ADC 模拟量值（0~4095） */
    uint16_t pwm_val;                   /* PWM 占空比值（0~1000）    */
    uint8_t  page_num;                  /* 当前页码（1/2/3/...）     */
    uint8_t  reserved[SAVE_RESERVED_NUM]; /* 预留字节，以后扩展用    */
} SaveData_t;


/* =========================================================
 *  函数声明
 * ========================================================= */

/**
 * @brief  上电初始化：从 Flash 读取保存的数据
 *         如果数据无效（第一次上电或数据损坏），返回默认值
 * @param  out_data : 读到的数据存在这里
 * @retval 1 = 读到了有效数据，0 = 用了默认值
 */
uint8_t Save_Init(SaveData_t *out_data);

/**
 * @brief  保存数据到 Flash（擦除第0扇区，重新写入）
 * @param  data : 要保存的数据
 * @retval 无
 */
void Save_Save(const SaveData_t *data);

/**
 * @brief  调试函数：读取 Flash 第0扇区开头的数据，串口打印出来
 *         平时注释掉不用，需要调试时取消注释即可
 * @retval 无
 */
void Save_DebugPrint(void);


#endif /* __APP_SAVE_H */
