#include "./BSP/CMD/cmd.h"
#include "./SYSTEM/usart/usart.h"
#include "./BSP/ADC/adc.h"
#include "./BSP/PWM/pwm.h"
#include "./BSP/LED/led.h"
#include "./APP/app_page_switch.h"
#include "./APP/app_save.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

SaveData_t g_save_data;
static uint8_t g_cmd_cnt = 5;   /* 当前命令数, 跟上面实际行数对齐 */

cmd_kv_t  g_kv_store[CMD_MAX_KV];   /* 全局键值对存储, 其他模块随时可读 */
uint8_t   g_kv_count = 0;            /* 当前已存储的键值对数量 */

volatile uint16_t  g_light_threshold = LIGHT_TH_DEFAULT;   /* 光敏电阻阈值 */
uint16_t  g_pwm_duty        = PWM_DEFAULT;        /* PWM 占空比 */

// 命令处理函数 (示例: 自己加新函数在这里写)
static void Cmd_Led_On(void)
{
    printf("LED ON   ");
    Led_Control(LIGHTNESS);
}

static void Cmd_Led_Off(void)
{
    printf("LED OFF   ");
    Led_Control(0);
}

static void Cmd_Read_Adc(void)
{
    printf("A:%.3fV, B:%.3fV, C:%.3fV LA: %d     ", filtered[0], filtered[1], filtered[2], adcx[1]);
}

/* ========= SAVE：保存数据到 Flash ========= */ 
static void Cmd_Handle_Save (void) { 
    /* 把当前最新的值填进去 */ 
    g_save_data.light_val = g_light_threshold; 
    /* 你当前的光敏值 */ 
    g_save_data.pwm_val   = g_pwm_duty; 
    /* 你当前的PWM值 */ 
    g_save_data.page_num  = current_page; 
    /* 你当前的页码 */ 
    Save_Save(&g_save_data); 
    printf ( "SAVE:OK" );
} 

/* ========= LOAD：从 Flash 读出来（调试用，可选） ========= */
static void Cmd_Handle_Load (void) {
    SaveData_t tmp; 
    uint8_t ok = Save_Init(&tmp); 
    if (ok) 
        printf ( "LOAD:OK light=%d pwm=%d page=%d\r\n" ,
               tmp.light_val, tmp.pwm_val, tmp.page_num); 
    else printf ( "LOAD:FAIL 数据无效，用默认值\r\n" );
}

/*
 *  命令表: 加新命令就在这里加一行
 *  格式: { "命令名", 函数名 },
*/
static const cmd_item_t G_Cmd_Table[CMD_MAX_NUM] =
{
    { "LED_ON",   Cmd_Led_On   },
    { "LED_OFF",  Cmd_Led_Off  },
    { READ_CHY_VOL, Cmd_Read_Adc },
    { "SAVE" ,  Cmd_Handle_Save }, /* ← 加这一行 */ 
    { "LOAD" ,  Cmd_Handle_Load }, /* ← 加这一行（可选，调试用） */
};

// 辅助函数: 判断字符是不是空白(空格/Tab/换行/回车)

static uint8_t Is_Space(uint8_t c)
{
    return (c == ' ' || c == '\t' || c == '\r' || c == '\n');
}

/*
 *  辅助函数: 从字符串里提取一个整数, 同时更新指针到数字末尾
 *  参数: s = 输入字符串起始地址
 *        endptr = 输出参数, 返回数字结束后的位置
 *  返回: 提取到的数值
 */

static int32_t My_Atoi_Ex(const char *s, const char **endptr)
{
    int32_t res = 0;
    uint8_t sign = 0;
    const char *p = s;

    if (*p == '-') { sign = 1; p++; }
    else if (*p == '+') { p++; }

    while (*p >= '0' && *p <= '9')
    {
        res = res * 10 + (*p - '0');
        p++;
    }

    /* 把结束位置写回调用者 */
    if (endptr) *endptr = p;

    return sign ? -res : res;
}

//执行单条命令, 返回 1=成功, 0=失败
static uint8_t Cmd_Exec_One(char *name)
{
    uint8_t i;

    for (i = 0; i < g_cmd_cnt; i++)
    {
        if (strcmp(name, G_Cmd_Table[i].name) == 0)
        {
            G_Cmd_Table[i].handler();   /* 匹配上了, 调函数 */
            return 1;
        }
    }

    /* 走到这里说明没匹配上 */
    printf("ERROR ORDER!!\r\n");
    return 0;
}

//判断一个 token 里有没有冒号
static uint8_t Token_Has_Colon(uint8_t *start, uint8_t *end)
{
    while (start < end)
    {
        if (*start == ':') return 1;
        start++;
    }
    return 0;
}

/*
 *  解析单个键值对 token, 如 "TH:52"
 *  参数: token_start = token 起始地址
 *        token_end = token 结束地址(不含空白)
 *        kv = 输出键值对
 *  返回: 1=成功解析, 0=格式不对
*/

static uint8_t Parse_Single_Kv(uint8_t *token_start, uint8_t *token_end, cmd_kv_t *kv)
{
    uint8_t *colon = token_start;
    const char *num_end;

    /* 找冒号 */
    while (colon < token_end && *colon != ':') colon++;

    if (colon >= token_end) return 0;  /* 多重保险 */

    /* 复制键名 */
    uint8_t klen = (uint8_t)(colon - token_start);
    if (klen > 7) klen = 7;
    uint8_t i;
    for (i = 0; i < klen; i++) kv->key[i] = token_start[i];
    kv->key[klen] = '\0';

    /* 冒号右边是数值, 用 My_Atoi_Ex 提取并更新指针 */
    const char *num_start = (const char *)(colon + 1);
    kv->val = My_Atoi_Ex(num_start, &num_end);
    kv->found = 1;

    return 1;
}

/*
 *  把一个键值对存入全局存储 g_kv_store
 *  如果键名已存在 → 更新值; 不存在 → 追加新条目
 *  返回: 1=成功, 0=存储满了
 */

static uint8_t Kv_Store_Save(cmd_kv_t *kv)
{
    uint8_t i;

    /* 先查有没有同名的键, 有就更新 */
    for (i = 0; i < g_kv_count; i++)
    {
        if (strcmp(g_kv_store[i].key, kv->key) == 0)
        {
            g_kv_store[i].val = kv->val;
            g_kv_store[i].found = 1;
            return 1;
        }
    }

    /* 没找到 → 追加新的 */
    if (g_kv_count >= CMD_MAX_KV) return 0;   /* 存满了 */
    memcpy(&g_kv_store[g_kv_count], kv, sizeof(cmd_kv_t));
    g_kv_count++;
    return 1;
}

/*
 *  快捷取值: 根据键名从全局存储里找数值
 *  找到返回 1, val 被赋值; 没找到返回 0
 */

uint8_t Kv_Get_Val(const char *key, int32_t *val)
{
    uint8_t i;
    for (i = 0; i < g_kv_count; i++)
    {
        if (strcmp(g_kv_store[i].key, key) == 0)
        {
            if (val) *val = g_kv_store[i].val;
            return 1;
        }
    }
    return 0;
}

/*
 *  处理光敏阈值键值对 (LIGHT:xxx)
 *  解析到 LIGHT 键时自动调用, 做范围检查并更新 g_light_threshold
 */

static void Kv_Handle_Light(int32_t val)
{
    if (val < 0)    val = 0;       /* ADC 原始值最小 0 */
    if (val > 4095) val = 4095;    /* 12 位 ADC 最大 4095 */
    g_light_threshold = (uint16_t)val;
    printf("LIGHT:%d ", g_light_threshold);
}

/*
 *  处理 PWM 占空比键值对 (PWM:xxx)
 *  解析到 PWM 键时自动调用, 做范围检查并更新 g_pwm_duty
 */
static void Kv_Handle_Pwm(int32_t val)
{
    if (val < 0)       val = 0;
    if (val > PWM_MAX) val = PWM_MAX;
    g_pwm_duty = (uint16_t)val;
    printf("PWM:%d ", g_pwm_duty);
    Led_Control(g_pwm_duty);
}

static void Kv_Handle_Page(int32_t val)
{
    if (val < 0)       val = 0;
    if (val > PAGE_TOTAL) val = PAGE_TOTAL;
    current_page = (uint16_t)val;
    DrawPage_Static(current_page);
    printf("Page:%d ", current_page);
}

/*
 *  根据键名分发到对应的处理函数
 *  目前支持: LIGHT / PWM
 *  加新的键值类型就在这里加 case
 */
static uint8_t Kv_Dispatch(cmd_kv_t *kv)
{
    if (strcmp(kv->key, LIGHT_TH_KEY) == 0)      /* LIGHT:xxx → 光敏阈值 */
    {
        Kv_Handle_Light(kv->val);
        return 1;
    }
    else if (strcmp(kv->key, PWM_KEY) == 0)      /* PWM:xxx → PWM 占空比 */
    {
        Kv_Handle_Pwm(kv->val);
        return 1;
    }
    else if(strcmp(kv ->key, PAGE_ORDER) == 0)
    {
        Kv_Handle_Page(kv->val);
        return 1;
    }
    else                                          /* 其他键 → 先存起来, 调试打印 */
    {
        Kv_Store_Save(kv);
        printf("%s:%d ", kv->key, kv->val);
        return 1;
    }
}

/*
 *  解析一帧数据里的所有键值对 (保留这个函数给外部调用)
 *  参数: buf = 数据起始
 *        len = 数据长度
 *        kv = 输出数组
 *        kv_max = 数组最大个数
 *  返回: 解析到的键值对个数
 */

uint8_t Cmd_Parse_Kv(uint8_t *buf, uint16_t len, cmd_kv_t *kv, uint8_t kv_max)
{
    uint8_t *p = buf;
    uint8_t *end = buf + len;
    uint8_t cnt = 0;
    uint8_t i;

    /* 先全部清零 */
    for (i = 0; i < kv_max; i++)
    {
        kv[i].key[0] = '\0';
        kv[i].val = 0;
        kv[i].found = 0;
    }

    while (p < end && cnt < kv_max)
    {
        /* 跳过空白 */
        while (p < end && Is_Space(*p)) p++;
        if (p >= end) break;

        /* 找这个 token 的结束位置 */
        uint8_t *token_start = p;
        while (p < end && !Is_Space(*p)) p++;
        uint8_t *token_end = p;

        /* 只解析含冒号的 token, 不含冒号的跳过 */
        if (Token_Has_Colon(token_start, token_end))
        {
            if (Parse_Single_Kv(token_start, token_end, &kv[cnt]))
            {
                cnt++;
            }
        }
    }

    return cnt;
}

/*
 *  处理一帧数据(主循环里调用)
 *  功能:
 *    1. 按空格切分成 token, 逐个判断
 *    2. 含冒号的 token → 键值对解析 → 存入全局 + 分发处理
 *    3. 不含冒号的 token → 命令表匹配
 *    4. 返回 1=全部成功, 0=有错误
 */

uint8_t Cmd_Process_Line(uint8_t *buf, uint16_t len)
{
    char name[CMD_NAME_MAX_LEN];
    uint8_t *p = buf;
    uint8_t *end = buf + len;
    uint8_t nlen;
    uint8_t all_ok = 1;   /* 记录是否全部成功 */

    /* 保证结尾有 \0, 否则 strcmp 会越界 */
    if (len >= RXBUFFERSIZE  - 1) len = RXBUFFERSIZE - 1;
    buf[len] = '\0';

    /* 逐个 token 处理, 不再整行走一条路 */
    while (p < end)
    {
        /* 跳过开头空白 */
        while (p < end && Is_Space(*p)) p++;
        if (p >= end) break;

        /* 找这个 token 的结束位置 */
        uint8_t *token_start = p;
        while (p < end && !Is_Space(*p)) p++;
        uint8_t *token_end = p;

        /* 复制到 name 缓冲区, 补 \0 */
        nlen = (uint8_t)(token_end - token_start);
        if (nlen >= CMD_NAME_MAX_LEN) nlen = CMD_NAME_MAX_LEN - 1;
        memcpy(name, token_start, nlen);
        name[nlen] = '\0';

        if (nlen == 0) continue;

        /* 判断这个 token 是键值对还是命令 */
        if (Token_Has_Colon(token_start, token_end))
        {
            /* 含冒号 → 键值对 */
            cmd_kv_t kv;
            if (Parse_Single_Kv(token_start, token_end, &kv))
            {
                Kv_Dispatch(&kv);   /* 分发到对应处理函数 */
            }
            else
            {
                printf("ERROR ORDER!!\r\n");
                all_ok = 0;
            }
        }
        else
        {
            /* 不含冒号 → 命令表匹配 */
            if (!Cmd_Exec_One(name))
            {
                all_ok = 0;   /* 有一个失败就标记 */
            }
        }
    }

    return all_ok;   /* 返回成功/失败状态 */
}

/*
 *  初始化
 */

void Cmd_Init(void)
{
    uint8_t i;

    /* 清空全局键值对存储 */
    for (i = 0; i < CMD_MAX_KV; i++)
    {
        g_kv_store[i].key[0] = '\0';
        g_kv_store[i].val = 0;
        g_kv_store[i].found = 0;
    }
    g_kv_count = 0;

    g_light_threshold = LIGHT_TH_DEFAULT;
    g_pwm_duty        = PWM_DEFAULT;
}
