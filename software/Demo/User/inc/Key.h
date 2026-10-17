#ifndef __KEY_H
#define __KEY_H

#include "stm32f0xx.h"

/* 按键事件 */
typedef enum {
    KEY_NONE = 0,   /* 无事件 */
    KEY1_SHORT,     /* KEY1 短按（松开触发） */
    KEY2_SHORT,     /* KEY2 短按（松开触发） */
    KEY1_LONG       /* KEY1 长按 3 秒（按下持续触发） */
} KeyEvent;

void Key_Init(void);        /* 初始化按键 GPIO + 1ms 计时基准 */
KeyEvent Key_Scan(void);    /* 主循环周期调用，返回一个按键事件 */

uint16_t Key_Millis(void);  /* 1ms 时基（TIM14 自由运行），供其它模块测时间 */

#endif
