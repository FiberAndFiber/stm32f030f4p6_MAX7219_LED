#ifndef __MAX7219_H
#define __MAX7219_H

#include "stm32f0xx.h"                  // Device header
#include "MAX7219_font.h"

/* ==================== MAX7219 寄存器 ==================== */
#define MAX7219_REG_NOOP         0x00
#define MAX7219_REG_DIGIT0       0x01
#define MAX7219_REG_DIGIT1       0x02
#define MAX7219_REG_DIGIT2       0x03
#define MAX7219_REG_DIGIT3       0x04
#define MAX7219_REG_DIGIT4       0x05
#define MAX7219_REG_DIGIT5       0x06
#define MAX7219_REG_DIGIT6       0x07
#define MAX7219_REG_DIGIT7       0x08
#define MAX7219_REG_DECODE       0x09
#define MAX7219_REG_INTENSITY    0x0A
#define MAX7219_REG_SCAN_LIMIT   0x0B
#define MAX7219_REG_SHUTDOWN     0x0C
#define MAX7219_REG_TEST         0x0F

/* ==================== 显示方向 ==================== */
#define ORIENT_NORMAL            0  // 正常
#define ORIENT_ROTATE_90         1  // 顺时针旋转 90°
#define ORIENT_ROTATE_180        2  // 旋转 180°
#define ORIENT_ROTATE_270        3  // 顺时针旋转 270°
#define ORIENT_MIRROR_H          4  // 水平镜像
#define ORIENT_MIRROR_V          5  // 垂直镜像

/* ==================== 滚动模式 ==================== */
#define SCROLL_RIGHT_TO_LEFT     0
#define SCROLL_LEFT_TO_RIGHT     1
#define SCROLL_BOTTOM_TO_TOP     2
#define SCROLL_TOP_TO_BOTTOM     3

/* ==================== 滚动缓冲区最大宽度 ==================== */
#define SCROLL_BUF_MAX_WIDTH     256

typedef struct {
		uint8_t data[8];
} MAX7219_Picture;


/* ==================== API 函数声明 ==================== */
void MAX7219_Init(void);
void MAX7219_WriteReg(uint8_t reg, uint8_t data);
void MAX7219_SetBrightness(uint8_t level);
void MAX7219_SetOrientation(uint8_t orientation);
void MAX7219_Clear(void);

void MAX7219_DrawBuffer(const uint8_t buf[8]);

void MAX7219_POINT(uint8_t x, uint8_t y);
void MAX7219_CLEARPOINT(uint8_t x, uint8_t y);
void MAX7219_SHOWNUM(uint8_t x, uint8_t y, uint8_t InputNum);
void MAX7219_SHOWCHAR(uint8_t x, uint8_t y, char InputChar);
//void MAX7219_SHOWSTR(const char *str, uint8_t len, uint16_t speed_ms, uint8_t mode);
void MAX7219_SHOWPICTURE(const MAX7219_Picture *frames, uint8_t frameCount, uint16_t interval_ms, uint8_t repeat);
void Diag_Map(void);
#endif
