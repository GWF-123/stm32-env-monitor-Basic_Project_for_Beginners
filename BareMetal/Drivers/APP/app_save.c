/**
 * @file     app_save.c
 * @brief    掉电保存 —— 把配置参数存到 W25Q128 里
 *           简单版：每次 SAVE 擦除第 0 扇区，重新写入
 *           （W25Q128 扇区寿命约 10 万次，手动保存完全够用）
 *
 * 数据结构（16字节，存在第0扇区开头）：
 *   字节  0~1  : magic    = 0x5356  （有效标志）
 *   字节  2~3  : light_val          （光敏ADC值）
 *   字节  4~5  : pwm_val            （PWM占空比）
 *   字节  6    : page_num           （当前页码）
 *   字节  7~10 : reserved[4]        （预留）
 *   字节 11    : checksum           （校验和）
 *   字节 12~15: 0xFF 填充
 */

#include "./APP/app_save.h"
#include "./NORFLASH/norflash.h"
#include <stdio.h>
#include <string.h>


/* =========================================================
 *  内部宏 / 默认值
 * ========================================================= */

#define SAVE_BUF_SIZE       16      /* 一条保存数据占 16 字节 */

/* 默认值（第一次上电或数据损坏时用） */
#define DEFAULT_LIGHT       2000
#define DEFAULT_PWM         500
#define DEFAULT_PAGE        1


/* =========================================================
 *  内部辅助函数：计算校验和
 *  方法：把 buf[0] ~ buf[len-1] 累加，取低 8 位
 * ========================================================= */
static uint8_t save_calc_checksum(uint8_t *buf, uint8_t len)
{
    uint16_t sum = 0;
    uint8_t  i;

    for (i = 0; i < len; i++)
    {
        sum += buf[i];
    }

    return (uint8_t)(sum & 0xFF);
}


/* =========================================================
 *  内部辅助函数：把 SaveData_t 打包成 16 字节数组
 *  返回值：1 = 打包成功
 * ========================================================= */
static void save_pack(const SaveData_t *data, uint8_t *buf)
{
    /* 先全部填 0xFF（Flash 擦除后就是 0xFF，保持一致） */
    memset(buf, 0xFF, SAVE_BUF_SIZE);

    /* 字节 0~1 : magic  0x5356 */
    buf[0] = (SAVE_MAGIC >> 8) & 0xFF;     /* 高字节 */
    buf[1] = SAVE_MAGIC & 0xFF;            /* 低字节 */

    /* 字节 2~3 : light_val */
    buf[2] = (data->light_val >> 8) & 0xFF;
    buf[3] = data->light_val & 0xFF;

    /* 字节 4~5 : pwm_val */
    buf[4] = (data->pwm_val >> 8) & 0xFF;
    buf[5] = data->pwm_val & 0xFF;

    /* 字节 6 : page_num */
    buf[6] = data->page_num;

    /* 字节 7~10 : reserved（预留，先填 0） */
    buf[7]  = data->reserved[0];
    buf[8]  = data->reserved[1];
    buf[9]  = data->reserved[2];
    buf[10] = data->reserved[3];

    /* 字节 11 : checksum（校验前 11 个字节） */
    buf[11] = save_calc_checksum(buf, 11);
}


/* =========================================================
 *  内部辅助函数：把 16 字节数组解包成 SaveData_t
 *  返回值：1 = 数据有效，0 = 数据无效（magic 错或校验失败）
 * ========================================================= */
static uint8_t save_unpack(const uint8_t *buf, SaveData_t *data)
{
    uint16_t magic;
    uint8_t  calc_chk;

    /* 1. 检查 magic：前两字节是不是 0x5356 强转成0053再<<8位*/
    magic = ((uint16_t)buf[0] << 8) | buf[1];
    if (magic != SAVE_MAGIC)
    {
        return 0;       /* magic 不对，数据无效 */
    }

    /* 2. 检查校验和 取低八位*/
    calc_chk = save_calc_checksum((uint8_t *)buf, 11);
    if (calc_chk != buf[11])
    {
        return 0;       /* 校验失败，数据可能损坏 */
    }

    /* 3. 解析数据 结构体指针*/
    data->light_val = ((uint16_t)buf[2] << 8) | buf[3];
    data->pwm_val   = ((uint16_t)buf[4] << 8) | buf[5];
    data->page_num  = buf[6];

    data->reserved[0] = buf[7];
    data->reserved[1] = buf[8];
    data->reserved[2] = buf[9];
    data->reserved[3] = buf[10];

    return 1;   /* 有效 */
}


/* =========================================================
 *  公开函数：上电初始化 —— 从 Flash 读数据
 *  返回值：1 = 读到有效数据，0 = 用了默认值
 * ========================================================= */
uint8_t Save_Init(SaveData_t *out_data)
{
    uint8_t buf[SAVE_BUF_SIZE];
    uint8_t ok;

    /* 从 Flash 第 0 扇区开头读 16 字节 */
    Norflash_Read(buf, SAVE_ADDR_START, SAVE_BUF_SIZE);

    /* 解包 + 校验 */
    ok = save_unpack(buf, out_data);

    if (ok == 0)
    {
        /* 数据无效（第一次上电或损坏），用默认值 */
        out_data->light_val = DEFAULT_LIGHT;
        out_data->pwm_val   = DEFAULT_PWM;
        out_data->page_num  = DEFAULT_PAGE;

        memset(out_data->reserved, 0, SAVE_RESERVED_NUM);

        return 0;
    }

    return 1;
}


/* =========================================================
 *  公开函数：保存数据到 Flash
 *  流程：打包成 16 字节 → 擦除第 0 扇区 → 写入
 *  简单的写入数据之前先擦除，并没有循环再擦除
 * ========================================================= */
void Save_Save(const SaveData_t *data)
{
    uint8_t buf[SAVE_BUF_SIZE];

    /* 1. 把结构体打包成字节数组 */
    save_pack(data, buf);

    /* 2. 擦除第 0 扇区
     *    （Flash 写之前必须先擦除，把所有位变回 1）
     */
    Norflash_Erase_Sector(SAVE_ADDR_START);

    /* 3. 写入 16 字节数据 */
    Norflash_Write(buf, SAVE_ADDR_START, SAVE_BUF_SIZE);
}


/* =========================================================
 *  公开函数：调试打印 —— 读 Flash 第 0 扇区，串口打印
 *  平时不用，需要调试时取消注释调用即可
 * ========================================================= */
void Save_DebugPrint(void)
{
    uint8_t  buf[SAVE_BUF_SIZE];
    SaveData_t data;
    uint8_t  ok;
    uint8_t  i;

    printf("\r\n===== W25Q128 第0扇区数据调试 =====\r\n");

    /* 读 16 字节 */
    Norflash_Read(buf, SAVE_ADDR_START, SAVE_BUF_SIZE);

    /* 打印原始字节 */
    printf("原始数据: ");
    for (i = 0; i < SAVE_BUF_SIZE; i++)
    {
        printf("%02X ", buf[i]);
    }
    printf("\r\n");

    /* 尝试解包 */
    ok = save_unpack(buf, &data);
    if (ok)
    {
        printf("【数据有效】\r\n");
        printf("  光敏(LIGHT): %d\r\n", data.light_val);
        printf("  PWM值     : %d\r\n", data.pwm_val);
        printf("  当前页码   : %d\r\n", data.page_num);
        printf("  预留字节   : %02X %02X %02X %02X\r\n",
               data.reserved[0], data.reserved[1],
               data.reserved[2], data.reserved[3]);
    }
    else
    {
        printf("【数据无效】第一次上电或数据已损坏，使用默认值\r\n");
    }

    printf("====================================\r\n\r\n");
}
