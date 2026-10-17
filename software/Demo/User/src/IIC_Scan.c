#include "stm32f0xx.h"

#include "SOFT_I2C_DRIVER.h"
#include "OLED.h"

#include "Delay.h"

/**
* @brief  检测某个 7 位地址上是否有设备应答
* @param  addr 7 位从机地址（0x00~0x7F）
* @retval 1 = 有设备（收到 ACK）  0 = 无设备（NACK）
*/
uint8_t I2C_Scan_CheckAddress(uint8_t addr)
{
    SOFTWARE_I2C_Start();
    SOFTWARE_I2C_SendByte((uint8_t)(addr << 1)); // 地址左移1位 + 写位(R/W=0)
    uint8_t Ack = SOFTWARE_I2C_ReciveAnswer();   // 0=ACK(有设备)  1=NACK(无设备)
    SOFTWARE_I2C_End();

    return (Ack == 0) ? 1 : 0;
}

/**
* @brief  扫描 0x01~0x7F 所有地址，把找到的设备地址存入数组
* @param  AddrList 存放结果的数组（7 位地址）
* @param  MaxLen   AddrList 的最大容量
* @retval 找到的设备数量
*/
uint8_t I2C_Scan_ScanBus(uint8_t *AddrList, uint8_t MaxLen)
{
    uint8_t count = 0;
    uint16_t addr;

    for (addr = 0x01; addr <= 0x7F; addr++)     // 0x00 是广播地址，跳过
    {
        if (I2C_Scan_CheckAddress((uint8_t)addr))
        {
            if (count < MaxLen)
            {
                AddrList[count] = (uint8_t)addr;
            }
            count++;
        }
    }
    return count;
}

/**
* @brief  I2C 总线扫描，并把结果显示到 OLED
* @param  无
* @retval 无
*/
void I2C_Scan_Show(void)
{
    uint8_t AddrList[16] = {0};
    uint8_t Num = 0;
    uint8_t i;

    SOFTWARE_I2C_Init();    // 初始化 I2C 引脚（开漏 + 外部上拉）
    OLED_Init();            // 初始化 OLED（内部也会重新配一次相同引脚，不冲突）

    OLED_Clear();
    OLED_ShowString(1, 1, "I2C Scanner");

    Num = I2C_Scan_ScanBus(AddrList, 16);

    OLED_ShowString(2, 1, "Found:");
    OLED_ShowNum(2, 7, Num, 2);

    /* 每行最多显示 4 个地址（每个占 "0xHH" 共 4 列） */
    for (i = 0; i < Num && i < 8; i++)
    {
        uint8_t Line = 3 + (i / 4);          // 第 3 行或第 4 行
        uint8_t Col  = 1 + (i % 4) * 4;      // 1、5、9、13 列

        OLED_ShowString(Line, Col, "0x");
        OLED_ShowHexNum(Line, Col + 2, AddrList[i], 2);
    }
}
