#include "stm32f0xx.h"                  // Device header
#include "stm32f0xx_rcc.h"
#include "stm32f0xx_gpio.h"

#include "Key.h"

/* ==================== 可调参数 ==================== */

/* 系统时钟：用于把 TIM14 配成 1ms 基准。
   STM32F030 默认 48MHz；若你的板子实际时钟不是 48MHz，请改这里！ */
#define SYSCLK_HZ        48000000UL

/* 长按判定时长（毫秒） */
#define KEY_LONG_MS      3000UL

/* 去抖：连续 KEY_DEBOUNCE 次采样电平一致才认为稳定。 */
#define KEY_DEBOUNCE     2

/* ==================== 1ms 计时基准（TIM14 自由运行，无需中断） ==================== */

static void Key_TickInit(void)
{
    RCC->APB1ENR |= RCC_APB1ENR_TIM14EN;    /* 使能 TIM14 时钟（APB1） */

    TIM14->PSC = (uint16_t)(SYSCLK_HZ / 1000UL) - 1;  /* 48MHz -> 1kHz，每 1ms CNT+1 */
    TIM14->ARR = 0xFFFF;                     /* 自由运行，16 位，65.5s 回绕 */
    TIM14->EGR = TIM_EGR_UG;                 /* 立即载入 PSC/ARR */
    TIM14->CR1 |= TIM_CR1_CEN;               /* 启动计数器 */
}

uint16_t Key_Millis(void)
{
    return (uint16_t)TIM14->CNT;             /* 单位：ms */
}

static uint16_t Key_TickElapsed(uint16_t start)
{
    return (uint16_t)(Key_Millis() - start); /* 16 位无符号减法天然处理回绕 */
}

/* ==================== 按键去抖状态 ==================== */

typedef struct {
    uint8_t stable;     /* 消抖后电平：1=松开，0=按下 */
    uint8_t prev;       /* 上一拍消抖电平（用于边沿检测） */
    uint8_t lastRaw;    /* 上次原始电平 */
    uint8_t cnt;        /* 去抖计数 */
} KeyDeb;

static KeyDeb k1;
static KeyDeb k2;

static uint8_t  k1_longFired;   /* 本次按下是否已上报过长按 */
static uint16_t k1_pressAt;     /* 本次按下起始 Tick */

/* 读原始电平：上拉输入，按下接地 → 按下为 0 */
static uint8_t Key_ReadRaw(uint16_t pin)
{
    return GPIO_ReadInputDataBit(GPIOA, pin);
}

/* 去抖：返回当前稳定电平（1=松开，0=按下） */
static uint8_t Key_Debounce(KeyDeb *k, uint16_t pin)
{
    uint8_t raw = Key_ReadRaw(pin);

    if (raw == k->lastRaw)
    {
        if (k->cnt < KEY_DEBOUNCE) k->cnt++;
    }
    else
    {
        k->cnt = 0;
        k->lastRaw = raw;
    }

    if (k->cnt >= KEY_DEBOUNCE) k->stable = raw;
    return k->stable;
}

void Key_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    Key_TickInit();

    /* KEY1=PA1，KEY2=PA2，上拉输入 */
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA, ENABLE);

    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_1 | GPIO_Pin_2;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IN;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;   /* 上拉，按下接地 */
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    k1.stable = 1; k1.prev = 1; k1.lastRaw = 1; k1.cnt = 0;
    k2.stable = 1; k2.prev = 1; k2.lastRaw = 1; k2.cnt = 0;
    k1_longFired = 0;
    k1_pressAt   = 0;
}

KeyEvent Key_Scan(void)
{
    uint8_t d1 = Key_Debounce(&k1, GPIO_Pin_1);
    uint8_t d2 = Key_Debounce(&k2, GPIO_Pin_2);

    uint8_t e1 = (d1 == 0 && k1.prev == 1);  /* KEY1 按下沿 */
    uint8_t r1 = (d1 == 1 && k1.prev == 0);  /* KEY1 松开沿 */
    uint8_t r2 = (d2 == 1 && k2.prev == 0);  /* KEY2 松开沿 */

    k1.prev = d1;
    k2.prev = d2;

    /* KEY2 只做短按（松开沿触发） */
    if (r2) return KEY2_SHORT;

    /* KEY1：短按 + 长按 */
    if (d1 == 0)   /* 按下中 */
    {
        if (e1)    /* 新的一次按下 */
        {
            k1_pressAt   = Key_Millis();
            k1_longFired = 0;
        }
        if (!k1_longFired && Key_TickElapsed(k1_pressAt) >= KEY_LONG_MS)
        {
            k1_longFired = 1;
            return KEY1_LONG;
        }
    }
    else   /* 松开中 */
    {
        if (r1 && !k1_longFired)   /* 松开沿且没触发过长按 → 短按 */
        {
            return KEY1_SHORT;
        }
    }

    return KEY_NONE;
}
