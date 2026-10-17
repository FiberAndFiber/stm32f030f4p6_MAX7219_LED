#include "stm32f0xx.h"                  // Device header

#include "MPU6050.h"
#include "MAX7219.h"
#include "FluidSim.h"

/* MPU6050 加速度计寄存器地址（用现有的 MPU6050_ReadReg 读，无需改 MPU6050.c） */
#define MPU6050_ACCEL_XOUT_H  0x3B
#define MPU6050_ACCEL_XOUT_L  0x3C
#define MPU6050_ACCEL_YOUT_H  0x3D
#define MPU6050_ACCEL_YOUT_L  0x3E
#define MPU6050_ACCEL_ZOUT_H  0x3F
#define MPU6050_ACCEL_ZOUT_L  0x40

/* ==================== 屏幕轴映射（关键） ====================
   点阵屏（板子）竖直放置，加速度计 X 轴 = 重力方向（屏内“下”）。
   屏内另一个轴（“右”）到底是 Y 还是 Z，取决于你 MPU 的安装朝向。

   ★ 若“横过来没反应 / 方向不对”，先把 HORIZ_USE_Z 取反试试。 */
#define HORIZ_USE_Z   0        /* 0=屏内水平轴用 Y；1=屏内水平轴用 Z */

/* 方向符号：若粒子往反方向堆积，把对应符号取反 */
#define GX_DIR  (-1)           /* 加速度计 +X(下) → 屏内 row 方向 */
#define GY_DIR  (-1)           /* 屏内水平轴(右) → 屏内 col 方向 */

/* ==================== 可调参数 ==================== */
#define FLUID_PASSES     1      /* 每次 Fluid_Step 移动子步数（越大越快，1=最平滑） */
#define FLUID_SPEED      256    /* 速度：8.8 定点，256=1格/子步（1g 时） */
#define FLUID_FLAT_G2    0.0144f /* 平放判定：(下²+右²) 低于此值不动，≈0.12g */
#define ACCEL_ALPHA      0.8f   /* 低通系数：越大响应越快、越抖（0~1） */

/* ==================== 姿态结算 ====================
   突发读 6 字节加速度计（一次 I2C 事务），快速一阶低通估计屏内重力矢量。
*/
static float g_down  = 1.0f;    /* 屏内“下”分量（g） */
static float g_right = 0.0f;    /* 屏内“右”分量（g） */

static void Fluid_UpdateAttitude(void)
{
    int16_t AX, AY, AZ;
    float down, right;

    /* 读加速度计三轴（用现有 MPU6050_ReadReg，6 个字节） */
    AX = (int16_t)((MPU6050_ReadReg(MPU6050_ACCEL_XOUT_H) << 8) |
                    MPU6050_ReadReg(MPU6050_ACCEL_XOUT_L));
    AY = (int16_t)((MPU6050_ReadReg(MPU6050_ACCEL_YOUT_H) << 8) |
                    MPU6050_ReadReg(MPU6050_ACCEL_YOUT_L));
    AZ = (int16_t)((MPU6050_ReadReg(MPU6050_ACCEL_ZOUT_H) << 8) |
                    MPU6050_ReadReg(MPU6050_ACCEL_ZOUT_L));

    down  = GX_DIR * (AX / 2048.0f);                       /* ±16g：2048 LSB/g */
    right = GY_DIR * ((HORIZ_USE_Z ? AZ : AY) / 2048.0f);

    g_down  += (down  - g_down)  * ACCEL_ALPHA;
    g_right += (right - g_right) * ACCEL_ALPHA;
}

/* ==================== 粒子状态（子像素 8.8 定点） ==================== */

static uint8_t fluid_count = FLUID_PARTICLE_DEFAULT;

static int16_t ppx[FLUID_PARTICLE_MAX];   /* col 方向，1格=256，子像素位置 */
static int16_t ppy[FLUID_PARTICLE_MAX];   /* row 方向，1格=256 */
static uint8_t grid[8][8];                /* 占用表：1=有粒子 */

static uint32_t rng_state = 0x9E3779B9UL;
static uint32_t rng(void)
{
    rng_state ^= rng_state << 13;
    rng_state ^= rng_state >> 17;
    rng_state ^= rng_state << 5;
    return rng_state;
}

/* ==================== 移动单个粒子 ==================== */

static void Fluid_MoveOne(uint8_t pi, int16_t step_col, int16_t step_row)
{
    int16_t px = ppx[pi];
    int16_t py = ppy[pi];
    int8_t  cc = (int8_t)(px >> 8);
    int8_t  cr = (int8_t)(py >> 8);

    int16_t nx = px + step_col;
    int16_t ny = py + step_row;
    int8_t  nc = (int8_t)(nx >> 8);
    int8_t  nr = (int8_t)(ny >> 8);

    if (nc == cc && nr == cr)          /* 同一格内漂移 */
    {
        ppx[pi] = nx;
        ppy[pi] = ny;
        return;
    }

    if (nc >= 0 && nc <= 7 && nr >= 0 && nr <= 7 && grid[nc][nr] == 0)  /* 进入空格 */
    {
        grid[cc][cr] = 0;
        grid[nc][nr] = 1;
        ppx[pi] = nx;
        ppy[pi] = ny;
        return;
    }

    /* 被挡 → 沿重力方向左右/上下滑一格 */
    {
        int8_t dCol = (step_col > 0) ? 1 : (step_col < 0 ? -1 : 0);
        int8_t dRow = (step_row > 0) ? 1 : (step_row < 0 ? -1 : 0);
        int8_t sd[2][2];
        uint8_t ns = 0, i;

        if (dCol == 0)          /* 垂直重力 → 左右斜落 */
        {
            sd[ns][0] = -1; sd[ns][1] = dRow; ns++;
            sd[ns][0] =  1; sd[ns][1] = dRow; ns++;
        }
        else if (dRow == 0)     /* 水平重力 → 上下斜落 */
        {
            sd[ns][0] = dCol; sd[ns][1] = -1; ns++;
            sd[ns][0] = dCol; sd[ns][1] =  1; ns++;
        }
        else                    /* 对角重力 → 两轴分量 */
        {
            sd[ns][0] = dCol; sd[ns][1] = 0; ns++;
            sd[ns][0] = 0;    sd[ns][1] = dRow; ns++;
        }

        if (rng() & 1)
        {
            int8_t t;
            t = sd[0][0]; sd[0][0] = sd[1][0]; sd[1][0] = t;
            t = sd[0][1]; sd[0][1] = sd[1][1]; sd[1][1] = t;
        }

        for (i = 0; i < ns; i++)
        {
            int8_t sc = cc + sd[i][0];
            int8_t sr = cr + sd[i][1];
            if (sc >= 0 && sc <= 7 && sr >= 0 && sr <= 7 && grid[sc][sr] == 0)
            {
                grid[cc][cr] = 0;
                grid[sc][sr] = 1;
                ppx[pi] = (sc << 8) + 128;
                ppy[pi] = (sr << 8) + 128;
                return;
            }
        }
    }

    ppx[pi] = (cc << 8) + 128;     /* 完全被挡：吸附到格心 */
    ppy[pi] = (cr << 8) + 128;
}

/* ==================== 渲染到 MAX7219 ==================== */

static void Fluid_Render(void)
{
    uint8_t buf[8] = {0};
    uint8_t i;
    for (i = 0; i < fluid_count; i++)
    {
        uint8_t c = (uint8_t)(ppx[i] >> 8);
        uint8_t r = (uint8_t)(ppy[i] >> 8);
        buf[r] |= (0x80 >> c);
    }
    MAX7219_DrawBuffer(buf);
}

/* ==================== 公开 API ==================== */

static void Fluid_Scatter(void)
{
    uint8_t cells[FLUID_PARTICLE_MAX];
    uint8_t i;

    for (i = 0; i < 64; i++) grid[i / 8][i % 8] = 0;

    for (i = 0; i < 64; i++) cells[i] = i;
    for (i = 64; i > 1; i--)
    {
        uint8_t j = (uint8_t)(rng() % i);
        uint8_t t = cells[i - 1]; cells[i - 1] = cells[j]; cells[j] = t;
    }

    for (i = 0; i < fluid_count; i++)
    {
        uint8_t cell = cells[i];
        uint8_t c = cell / 8;
        uint8_t r = cell % 8;
        ppx[i] = (c << 8) + 128;
        ppy[i] = (r << 8) + 128;
        grid[c][r] = 1;
    }

    g_down  = 1.0f;
    g_right = 0.0f;
    Fluid_Render();
}

void Fluid_Init(void)
{
    fluid_count = FLUID_PARTICLE_DEFAULT;
    Fluid_Scatter();
}

void Fluid_SetCount(uint8_t n)
{
    if (n < FLUID_PARTICLE_MIN) n = FLUID_PARTICLE_MIN;
    if (n > FLUID_PARTICLE_MAX) n = FLUID_PARTICLE_MAX;
    fluid_count = n;
    Fluid_Scatter();
}

void Fluid_Step(void)
{
    int16_t step_col, step_row;
    uint8_t order[FLUID_PARTICLE_MAX];
    uint8_t i, p, n = fluid_count;
    float mag2;

    Fluid_UpdateAttitude();

    mag2 = g_down * g_down + g_right * g_right;
    if (mag2 < FLUID_FLAT_G2) return;

    step_col = (int16_t)(g_right * FLUID_SPEED);
    step_row = (int16_t)(g_down  * FLUID_SPEED);
    if (step_col == 0 && step_row == 0) return;

    for (p = 0; p < FLUID_PASSES; p++)
    {
        for (i = 0; i < n; i++) order[i] = i;
        for (i = n; i > 1; i--)
        {
            uint8_t j = (uint8_t)(rng() % i);
            uint8_t t = order[i - 1]; order[i - 1] = order[j]; order[j] = t;
        }
        for (i = 0; i < n; i++)
        {
            Fluid_MoveOne(order[i], step_col, step_row);
        }
    }

    Fluid_Render();
}

/* ==================== 调试：显示重力方向 ====================
   在点阵上画一个亮点，位置 = 中心 + 重力方向×3.5。
   转动板子时，这个点应“立刻”跟着重力方向移动。
   若点不动 / 方向不对 → 是轴映射或传感器问题（试 HORIZ_USE_Z / GX_DIR / GY_DIR）。
   若点跟手很快、但粒子慢 → 是移动速度问题（调 FLUID_SPEED / FLUID_PASSES）。
*/
void Fluid_Debug(void)
{
    uint8_t buf[8] = {0};
    int col, row;

    Fluid_UpdateAttitude();

    col = 3 + (int)(g_right * 3.5f);
    row = 3 + (int)(g_down  * 3.5f);
    if (col < 0) col = 0; if (col > 7) col = 7;
    if (row < 0) row = 0; if (row > 7) row = 7;

    buf[row] |= (0x80 >> col);
    MAX7219_DrawBuffer(buf);
}
