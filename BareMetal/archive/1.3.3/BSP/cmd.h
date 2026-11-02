#ifndef __CMD_H
#define __CMD_H

#include <stdint.h>
#include "./SYSTEM/sys/sys.h"
#include "./APP/app_save.h"

/* 命令表最大条目数, 不够了调大 */
#define CMD_MAX_NUM         16

/* 键值对最大个数 */
#define CMD_MAX_KV          16

/* 单个命令名最大长度 */
#define CMD_NAME_MAX_LEN    16

/* PWM 调光相关 (默认值: 500, 范围 0~1000) */
#define PWM_KEY           "PWM"         /* 键名, 发 "PWM:500" 设置占空比 */
#define PWM_MAX           999
#define PWM_DEFAULT       0

/* 光敏阈值相关 (默认值: 3000, 低于此值认为"天黑"，模拟量越小关照越强，得设置大一点，模拟光照弱) */
#define LIGHT_TH_KEY      "LIGHT"       /* 键名, 发 "LIGHT:3000" 设置阈值 */
#define LIGHT_TH_DEFAULT   3000

#define READ_CHY_VOL      "READ_ADC"
#define PAGE_ORDER      "PAGE"
#define KEY_NAME_LENGTH     16 /* 键名, 如 "A" "B" "DUTY"，最多KEY_NAME_LENGTH个字符*/

/* 键值对结构体 */
typedef struct {
    char  key[KEY_NAME_LENGTH];       
    uint32_t  val;          /* 数值 */
    uint8_t  found;        /* 1 = 本帧里找到这个键 */
} cmd_kv_t;

/* 命令表项: 命令名 + 函数指针 */
typedef struct {
    char name[CMD_NAME_MAX_LEN];
    void (*handler)(void);
} cmd_item_t;

static void Kv_Handle_Save (void);
static void Kv_Handle_Load (void);
static const cmd_item_t G_Cmd_Table[CMD_MAX_NUM];

/* 键值对全局保存: 解析完后存这里, 其他模块随时可以读 */
extern cmd_kv_t  g_kv_store[CMD_MAX_KV];
extern uint8_t   g_kv_count;
extern uint16_t g_pwm_duty;              /* PWM 占空比 (0~PWM_MAX) */
volatile extern uint16_t g_light_threshold;       /* 光敏电阻阈值 (ADC 原始值) */
extern char g_cmd_tx_buf[128];

/* 快捷取值: 根据键名找数值, 找到返回1, val 被赋值; 没找到返回0 */
uint8_t  Kv_Get_Val(const char *key, int32_t *val);

/* 返回值: 1=全部成功, 0=有错误 这个是解析发送进来的字符串，然后进行修改值和对命令的解析*/
uint8_t Cmd_Process_Line(uint8_t *buf, uint16_t len);

uint8_t Cmd_Parse_Kv(uint8_t *buf, uint16_t len, cmd_kv_t *kv, uint8_t kv_max);

void Cmd_Init(void);//先初始化

#endif
