#ifndef __FLUIDSIM_H
#define __FLUIDSIM_H

#include "stm32f0xx.h"

/* ==================== 粒子数量（可方便修改） ==================== */
#define FLUID_PARTICLE_MIN      1    /* 下限 */
#define FLUID_PARTICLE_MAX      64   /* 上限（8×8 刚好铺满） */
#define FLUID_PARTICLE_DEFAULT  32   /* 缺省值 */

void Fluid_Init(void);               /* 初始化并随机散布粒子 */
void Fluid_SetCount(uint8_t n);      /* 修改粒子数量（自动夹到 1~64）并重新散布 */
void Fluid_Step(void);               /* 推进一步：读姿态 + 粒子运动 + 刷新显示 */

void Fluid_Debug(void);              /* 调试：在点阵上画一个点指示重力方向 */

#endif
