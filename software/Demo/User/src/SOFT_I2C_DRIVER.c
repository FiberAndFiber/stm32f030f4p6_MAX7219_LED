#include "stm32f0xx.h"                  // Device header
#include "stm32f0xx_rcc.h"
#include "stm32f0xx_gpio.h"

#include "Delay.h"

/*引脚配置
SCL：GPIOA,GPIO_Pin_3
SDA：GPIOA,GPIO_Pin_4
*/
void SOFTWARE_I2C_SCL(uint8_t BitValue)
{
    GPIO_WriteBit(GPIOA,GPIO_Pin_3,(BitAction)BitValue);
    Delay_us(10);
}

void SOFTWARE_I2C_SDA(uint8_t BitValue)
{
    GPIO_WriteBit(GPIOA,GPIO_Pin_4,(BitAction)BitValue);
    Delay_us(10);
}

/*read function*/
uint8_t SOFTWARE_I2C_SDA_R(void)
{
    uint8_t BitValue;
    BitValue = GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_4);
    Delay_us(10);
    return BitValue;
}

void SOFTWARE_I2C_Init(void)
{
  RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA,ENABLE);
	
	GPIO_InitTypeDef GPIO_Init_Structure;
	GPIO_Init_Structure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_Init_Structure.GPIO_OType = GPIO_OType_OD;   //开漏模式
	GPIO_Init_Structure.GPIO_PuPd = GPIO_PuPd_NOPULL;	//使用外部上拉，引脚无需上下拉

	GPIO_Init_Structure.GPIO_Pin = GPIO_Pin_3 | GPIO_Pin_4;
	GPIO_Init_Structure.GPIO_Speed=GPIO_Speed_50MHz;
	GPIO_Init(GPIOA,&GPIO_Init_Structure);	

  GPIO_SetBits(GPIOA,GPIO_Pin_3 | GPIO_Pin_4);
}

/*start*/
void SOFTWARE_I2C_Start(void)
{
    SOFTWARE_I2C_SDA(1);
    SOFTWARE_I2C_SCL(1);
    SOFTWARE_I2C_SDA(0);
    SOFTWARE_I2C_SCL(0);   
}

/*end*/
void SOFTWARE_I2C_End(void)
{
    SOFTWARE_I2C_SDA(0);
    SOFTWARE_I2C_SCL(1);
    SOFTWARE_I2C_SDA(1);    
}
/*1byte = 1B = 8bit*/
void SOFTWARE_I2C_SendByte(uint8_t Byte)
{
    for(uint8_t i = 0; i<8; i++){
        SOFTWARE_I2C_SDA(!!(Byte & (0x80 >> i))); 
        SOFTWARE_I2C_SCL(1);
        SOFTWARE_I2C_SCL(0);
    }    
}

uint8_t SOFTWARE_I2C_ReciveByte()
{   
    uint8_t Byte = 0x00;
    SOFTWARE_I2C_SDA(1);
    for(uint8_t i = 0;i<8;i++)
    {
        SOFTWARE_I2C_SCL(1);
        if(SOFTWARE_I2C_SDA_R()==1){Byte |= (0x80 >> i);}
        SOFTWARE_I2C_SCL(0);
    }
    return Byte;
}

void SOFTWARE_I2C_SendAnswer(uint8_t AnswerBit)
{
    SOFTWARE_I2C_SDA(AnswerBit); 
    SOFTWARE_I2C_SCL(1);
    SOFTWARE_I2C_SCL(0);
    
}

uint8_t SOFTWARE_I2C_ReciveAnswer()
{   
    uint8_t AnswerBit = 0x00;

    SOFTWARE_I2C_SDA(1);//release SDA
    SOFTWARE_I2C_SCL(1);
    AnswerBit = SOFTWARE_I2C_SDA_R();
    SOFTWARE_I2C_SCL(0);

    return AnswerBit;
}

/**
    * @brief  从指定 7 位地址的设备读一个寄存器
    * @param  addr 7 位从机地址（如 0x58、0x68）
    * @param  reg  寄存器地址
    * @retval 寄存器值（读不到设备时返回 0xFF）
*/
uint8_t I2C_ReadReg(uint8_t addr, uint8_t reg)
{
    uint8_t val;

    SOFTWARE_I2C_Start();
    SOFTWARE_I2C_SendByte((uint8_t)(addr << 1));      // 地址 + 写位
    SOFTWARE_I2C_ReciveAnswer();                       // 吞掉 ACK 时钟
    SOFTWARE_I2C_SendByte(reg);                        // 寄存器地址
    SOFTWARE_I2C_ReciveAnswer();

    SOFTWARE_I2C_Start();                              // 重复起始
    SOFTWARE_I2C_SendByte((uint8_t)((addr << 1) | 1)); // 地址 + 读位
    SOFTWARE_I2C_ReciveAnswer();

    val = SOFTWARE_I2C_ReciveByte();
    SOFTWARE_I2C_SendAnswer(1);                        // NACK，结束读
    SOFTWARE_I2C_End();

    return val;
}
