#include "stm32f0xx.h"                  // Device header

#include "SOFT_I2C_DRIVER.h"

#define AT24C02_WRITE_ADDRESS 0xA0
#define AT24C02_READ_ADDRESS  0xA1

/*寄存器地址*/
/*全部寄存器地址是从 0x00 到 0xFF，一共 256 个*/
/*256 字节 = 32 页 × 每页 8 字节*/

void AT24C02_WriteReg(uint8_t RegAddress , uint8_t Data)
{
    SOFTWARE_I2C_Start();
    SOFTWARE_I2C_SendByte(AT24C02_WRITE_ADDRESS);
    SOFTWARE_I2C_ReciveAnswer();
    SOFTWARE_I2C_SendByte(RegAddress);
    SOFTWARE_I2C_ReciveAnswer();
    SOFTWARE_I2C_SendByte(Data);
    SOFTWARE_I2C_ReciveAnswer();
    SOFTWARE_I2C_End();
}

/* example

AT24C02_WriteReg(0x00,211);//8位是0~255
Delay_ms(5);
uint8_t ReadData = AT24C02_ReadReg(0x00);
OLED_ShowNum(1,1,ReadData,5);

*/


/* example

uint16_t num = 1235;

//写：高字节存 0x00，低字节存 0x01
AT24C02_WriteReg(0x00, (uint8_t)(num >> 8));   // 高字节 = 4
Delay_ms(5);
AT24C02_WriteReg(0x01, (uint8_t)(num & 0xFF)); // 低字节 = 211
Delay_ms(5);

// 读：两个字节拼回来
uint16_t ReadData = ((uint16_t)AT24C02_ReadReg(0x00) << 8)
                |  (uint16_t)AT24C02_ReadReg(0x01);
// ReadData = (4 << 8) | 211 = 1024 + 211 = 1235

*/

uint8_t AT24C02_ReadReg(uint8_t RegAddress)
{   
    uint8_t Data;

    SOFTWARE_I2C_Start();
    SOFTWARE_I2C_SendByte(AT24C02_WRITE_ADDRESS);//用写模式设置地址指针，告诉芯片读哪个位置
    SOFTWARE_I2C_ReciveAnswer();
    SOFTWARE_I2C_SendByte(RegAddress);
    SOFTWARE_I2C_ReciveAnswer();
    
    SOFTWARE_I2C_Start();
    SOFTWARE_I2C_SendByte(AT24C02_READ_ADDRESS);//读取
    SOFTWARE_I2C_ReciveAnswer();
    Data = SOFTWARE_I2C_ReciveByte();  
    SOFTWARE_I2C_SendAnswer(1);
    SOFTWARE_I2C_End();

    return Data;
}

void AT24C02_Init(void)
{

}