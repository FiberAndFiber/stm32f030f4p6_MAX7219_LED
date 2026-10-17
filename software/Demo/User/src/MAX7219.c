#include "stm32f0xx.h"                  // Device header

#include "Delay.h"

#include "SOFT_SPI_DRIVER.h"
#include "MAX7219.h"

/* ==================== 模块内部变量 ==================== */

/* Framebuffer: 逻辑坐标系，8 字节，fb[row] 的 bit(7-col) 对应像素 (col, row)
	 左上角 = (0,0)，col=0~7，row=0~7
	 所有绘图函数均在此逻辑坐标系上操作，方向变换由 Flush 统一处理 */
static uint8_t MAX7219_FB[8];

/* 当前显示方向 */
static uint8_t MAX7219_Orientation = ORIENT_NORMAL;

/* ==================== 内部辅助函数 ==================== */

/* 发送 16 位（地址 + 数据）到 MAX7219 */
static void MAX7219_Send16(uint8_t addr, uint8_t data)
{
		SOFE_SPI_DRIVER_START();
		SOFE_SPI_DRIVER_SendByte(addr);
		SOFE_SPI_DRIVER_SendByte(data);
		SOFE_SPI_DRIVER_END();
}

/* ==================== 方向变换核心 ====================
	 将逻辑 framebuffer 按 orientation 映射到物理 DIGIT 寄存器并发送。
	 逻辑坐标系固定不变（所有绘图函数不受方向影响），只有此处做坐标变换。

	 物理坐标 (pCol, pRow) → 对应 DIGIT(pRow) 的 SEG bit(7-pCol)
	 变换方向: 物理 → 逻辑（即对于每个物理像素，从逻辑 FB 的哪个位置取数据）
*/
static void MAX7219_Flush(void)
{
		uint8_t pRow, pCol;        // 物理行列
		uint8_t logRow, logCol;    // 逻辑行列
		uint8_t physFB[8] = {0};   // 物理帧缓冲

		for (pRow = 0; pRow < 8; pRow++)
		{
				for (pCol = 0; pCol < 8; pCol++)
				{
						/* 根据方向，将物理坐标映射回逻辑坐标 */
						switch (MAX7219_Orientation)
						{
						default:
						case ORIENT_NORMAL:
								logCol = pCol;
								logRow = pRow;
								break;

						case ORIENT_ROTATE_90:   // 顺时针 90°
								logCol = pRow;
								logRow = 7 - pCol;
								break;

						case ORIENT_ROTATE_180:  // 旋转 180°
								logCol = 7 - pCol;
								logRow = 7 - pRow;
								break;

						case ORIENT_ROTATE_270:  // 顺时针 270°
								logCol = 7 - pRow;
								logRow = pCol;
								break;

						case ORIENT_MIRROR_H:    // 水平镜像
								logCol = 7 - pCol;
								logRow = pRow;
								break;

						case ORIENT_MIRROR_V:    // 垂直镜像
								logCol = pCol;
								logRow = 7 - pRow;
								break;
						}

						/* 从逻辑 framebuffer 取像素，写入物理 framebuffer */
						if (MAX7219_FB[logRow] & (0x80 >> logCol))
						{
								physFB[pRow] |= (0x80 >> pCol);
						}
				}
		}

		/* 将物理 framebuffer 发送到 MAX7219 */
		for (pRow = 0; pRow < 8; pRow++)
		{
				MAX7219_Send16(MAX7219_REG_DIGIT0 + pRow, physFB[pRow]);
		}
}

/* 从字库中取字符数据指针，返回 7 字节字模的首地址 */
static const uint8_t *MAX7219_GetFont(char ch)
{
		if (ch < 32 || ch > 126) ch = 32;
		return &MAX7219_Font[(uint16_t)(ch - 32) * 7];
}

/* 在逻辑 framebuffer 的 (cx, cy) 处写入一个像素 */
static void MAX7219_SetPixel(uint8_t cx, uint8_t cy)
{
		if (cx > 7 || cy > 7) return;
		MAX7219_FB[cy] |= (0x80 >> cx);
}

/* 在逻辑 framebuffer 的 (cx, cy) 处清除一个像素 */
static void MAX7219_ClrPixel(uint8_t cx, uint8_t cy)
{
		if (cx > 7 || cy > 7) return;
		MAX7219_FB[cy] &= ~(0x80 >> cx);
}

/* 将字模的某一列写入逻辑 framebuffer 的目标区域
	 colData: 字模列数据（bit0 = 该列最上方像素）
	 targetX: 该列在屏幕上的 x 坐标
	 targetY: 字符在屏幕上的 y 起点 */
static void MAX7219_PutColumn(uint8_t colData, uint8_t targetX, uint8_t targetY)
{
		uint8_t row;
		for (row = 0; row < 8; row++)
		{
				uint8_t ty = targetY + row;
				if (ty > 7) continue;

				if (colData & (0x01 << row))
						MAX7219_SetPixel(targetX, ty);
				else
						MAX7219_ClrPixel(targetX, ty);
		}
}

/* ==================== 公开 API ==================== */

/* 初始化 MAX7219 */
void MAX7219_Init(void)
{
		uint8_t i;

		SOFE_SPI_DRIVER_INIT();

		MAX7219_Send16(MAX7219_REG_SHUTDOWN,   0x00);
		MAX7219_Send16(MAX7219_REG_TEST,        0x00);
		MAX7219_Send16(MAX7219_REG_SCAN_LIMIT,  0x07);
		MAX7219_Send16(MAX7219_REG_DECODE,      0x00);
		MAX7219_Send16(MAX7219_REG_INTENSITY,   0x01);
		MAX7219_Send16(MAX7219_REG_SHUTDOWN,    0x01);

		MAX7219_Orientation = ORIENT_NORMAL;
		for (i = 0; i < 8; i++) MAX7219_FB[i] = 0x00;
		MAX7219_Flush();
}

/* 写寄存器（直接透传） */
void MAX7219_WriteReg(uint8_t reg, uint8_t data)
{
		MAX7219_Send16(reg, data);
}

/* 设置亮度 0~15 */
void MAX7219_SetBrightness(uint8_t level)
{
		if (level > 15) level = 15;
		MAX7219_Send16(MAX7219_REG_INTENSITY, level);
}

/* ==================== 设置显示方向 ====================
	 orientation:
		 ORIENT_NORMAL    (0) — 正常方向
		 ORIENT_ROTATE_90 (1) — 顺时针 90°
		 ORIENT_ROTATE_180(2) — 旋转 180°
		 ORIENT_ROTATE_270(3) — 顺时针 270°
		 ORIENT_MIRROR_H  (4) — 水平镜像
		 ORIENT_MIRROR_V  (5) — 垂直镜像
	 修改后立即刷新屏幕，现有画面按新方向重新显示。
*/
void MAX7219_SetOrientation(uint8_t orientation)
{
		if (orientation > 5) return;
		MAX7219_Orientation = orientation;
		MAX7219_Flush();  // 立即按新方向刷新
}

/* 清屏 */
void MAX7219_Clear(void)
{
		uint8_t i;
		for (i = 0; i < 8; i++) MAX7219_FB[i] = 0x00;
		MAX7219_Flush();
}

void MAX7219_DrawBuffer(const uint8_t buf[8])
{
				uint8_t i;
				for (i = 0; i < 8; i++) MAX7219_FB[i] = buf[i];
				MAX7219_Flush();
}

/* ==================== 点亮单个 LED ====================
	 (x, y): 逻辑坐标，左上角为 (0, 0)，x=列, y=行
	 不受方向设置影响——改变方向后，(0,0) 始终是"逻辑左上角"
*/
void MAX7219_POINT(uint8_t x, uint8_t y)
{
		if (x > 7 || y > 7) return;
		MAX7219_FB[y] |= (0x80 >> x);
		MAX7219_Flush();  // 逐点刷新（保证即时响应）
}

/* 熄灭单个 LED */
void MAX7219_CLEARPOINT(uint8_t x, uint8_t y)
{
		if (x > 7 || y > 7) return;
		MAX7219_FB[y] &= ~(0x80 >> x);
		MAX7219_Flush();
}

/* ==================== 显示一位数字 ====================
	 (x, y): 左上角坐标，InputNum: 0~9
	 数字统一占 4 列宽度（窄字符左侧补空白）
*/
void MAX7219_SHOWNUM(uint8_t x, uint8_t y, uint8_t InputNum)
{
		const uint8_t *font;
		uint8_t w, col;

		if (InputNum > 9) return;

		font = MAX7219_GetFont('0' + InputNum);
		w    = font[0];

		for (col = 0; col < 4; col++)
		{
				uint8_t tx = x + col;
				if (tx > 7) continue;

				if (col < w)
						MAX7219_PutColumn(font[2 + col], tx, y);
				else
						MAX7219_PutColumn(0x00, tx, y);
		}

		MAX7219_Flush();
}

/* ==================== 显示一个字符 ====================
	 (x, y): 左上角坐标，InputChar: ASCII 字符
	 统一占 4 列宽度
*/
void MAX7219_SHOWCHAR(uint8_t x, uint8_t y, char InputChar)
{
		const uint8_t *font;
		uint8_t w, col;

		font = MAX7219_GetFont(InputChar);
		w    = font[0];

		for (col = 0; col < 4; col++)
		{
				uint8_t tx = x + col;
				if (tx > 7) continue;

				if (col < w)
						MAX7219_PutColumn(font[2 + col], tx, y);
				else
						MAX7219_PutColumn(0x00, tx, y);
		}

		MAX7219_Flush();
}

  /* ==================== 显示图片 / 帧动画 ====================
     frames:      图片帧数组指针
     frameCount:  帧数量
     interval_ms: 帧间隔 (ms)
     repeat:      播放次数
                  0 — 无限循环播放（永不返回）
                  1 — 播放一轮（默认行为）
                  N — 播放 N 轮

     行为：
     - 若 frameCount == 1 或 interval_ms == 0:
         只显示第一帧（静态图片），忽略 repeat 参数，显示后立即返回
     - 其他情况按 repeat 参数控制播放轮数

     示例：
       // 静态显示一张图
       MAX7219_SHOWPICTURE(&icon, 1, 0, 1);

       // 循环播放两帧动画，播放 3 轮，每帧间隔 200ms
       MAX7219_SHOWPICTURE(anim, 2, 200, 3);

       // 无限循环播放
       MAX7219_SHOWPICTURE(anim, 2, 200, 0);
  */
void MAX7219_SHOWPICTURE(const MAX7219_Picture *frames, uint8_t frameCount, uint16_t interval_ms, uint8_t repeat)
{
		uint8_t i, row;
		uint8_t round;

		if (frames == 0 || frameCount == 0) return;

		/* 单帧或无间隔 → 静态显示第一帧后返回 */
		if (frameCount == 1 || interval_ms == 0)
		{
				for (row = 0; row < 8; row++)
				{
						MAX7219_FB[row] = frames[0].data[row];
				}
				MAX7219_Flush();
				return;
		}

		/* 多帧动画 → 按 repeat 控制播放轮数
			 repeat == 0 表示无限循环（永不返回）
			 repeat >= 1 表示播放指定轮数        */
		round = 0;
		while (repeat == 0 || round < repeat)
		{
				for (i = 0; i < frameCount; i++)
				{
						/* 将当前帧数据拷贝到 framebuffer */
						for (row = 0; row < 8; row++)
						{
								MAX7219_FB[row] = frames[i].data[row];
						}
						MAX7219_Flush();

						/* 帧间隔延时 */
						for (volatile uint32_t d = 0; d < interval_ms * 1000; d++)
						{
								__NOP();
						}
				}
				round++;
		}
}

void Diag_Map(void)
{
		uint8_t i, k;

		/* 第1组：DIG0~DIG7 逐个全亮(0xFF)，其余灭 */
		for (i = 0; i < 8; i++)
		{
				for (k = 0; k < 8; k++)
						MAX7219_WriteReg(MAX7219_REG_DIGIT0 + k, 0x00);  // 全灭
				MAX7219_WriteReg(MAX7219_REG_DIGIT0 + i, 0xFF);       // DIG[i] 全亮
				Delay_ms(100);
		}

		/* 第2组：bit0~bit7 逐个，所有 DIG 都亮这一位 */
		for (i = 0; i < 8; i++)
		{
				for (k = 0; k < 8; k++)
						MAX7219_WriteReg(MAX7219_REG_DIGIT0 + k, 0x01 << i);
				Delay_ms(100);
		}

		for (k = 0; k < 8; k++)
				MAX7219_WriteReg(MAX7219_REG_DIGIT0 + k, 0x00);       // 收尾全灭
}
