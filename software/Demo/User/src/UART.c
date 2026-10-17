#include "stm32f0xx.h"                  // Device header
#include "stm32f0xx_rcc.h"
#include "stm32f0xx_gpio.h"

uint8_t Serial_RxData;
uint8_t Serial_RxFlag;

void Serial_Init(void)
{
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_USART1,ENABLE);
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA,ENABLE);

    GPIO_InitTypeDef GPIO_InitStructure;
    /*TX*/
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;    //推挽输出
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA,&GPIO_InitStructure);
    /*RX*/
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;    //上拉输入
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA,&GPIO_InitStructure);
        
    USART_InitTypeDef USART_InitStructure;
    USART_InitStructure.USART_BaudRate = 9600;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None; //无硬件流控制
    USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;					//发送模式
    USART_InitStructure.USART_Parity = USART_Parity_No;					//无校验位
    USART_InitStructure.USART_StopBits = USART_StopBits_1;		//1s位数据位
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;	//8位数据位
    USART_Init(USART1,&USART_InitStructure);

    /*配置中断*/
    USART_ITConfig(USART1,USART_IT_RXNE,ENABLE);

    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);

    NVIC_InitTypeDef NVIC_InitStructure;
    NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
  	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_Init(&NVIC_InitStructure);

    USART_Cmd(USART1,ENABLE);
}

void Serial_SendByte(uint8_t Byte)
{
  USART_SendData(USART1,Byte);
  while (USART_GetFlagStatus(USART1,USART_FLAG_TXE) == RESET);
}

void Serial_SendArray(uint8_t *Array,uint16_t Length)
{
    uint8_t i;
    for(i = 0; i < Length ; i++)
    {
      Serial_SendByte(Array[i]);
    }
}

void Serial_SendString(char *String)
{
    uint8_t i;
    for(i = 0;String[i] != '\0'; i++)
    {
        Serial_SendByte(String[i]);
    }
}

uint32_t Serial_Pow(uint32_t x,uint32_t y)//计算x的y次方
{
    uint32_t Result = 1;
    while(y--)
    {
        Result *= x;
    }
    return Result;
}

void Serial_SendNumber(uint32_t Number,uint8_t Length)
{
    uint8_t i;
    for (i = 0;i < Length; i++)
    {
        Serial_SendByte(Number / Serial_Pow(10,Length - i - 1) % 10+0x30);
    }
}

/**
  * 函    数：获取串口接收标志位
  * 参    数：无
  * 返 回 值：串口接收标志位，范围：0~1，接收到数据后，标志位置1，读取后标志位自动清零
  */
uint8_t Serial_GetRxFlag(void)
{
	if (Serial_RxFlag == 1)			//如果标志位为1
	{
		Serial_RxFlag = 0;
		return 1;					//则返回1，并自动清零标志位
	}
	return 0;						//如果标志位为0，则返回0
}

/**
  * 函    数：获取串口接收的数据
  * 参    数：无
  * 返 回 值：接收的数据，范围：0~255
  */
uint8_t Serial_GetRxData(void)
{
	return Serial_RxData;			//返回接收的数据变量
}

void USART1_IRQHandler(void)
{
    if (USART_GetITStatus(USART1,USART_IT_RXNE) == SET)
    {
        Serial_RxData = USART_ReceiveData(USART1);      //读取数据寄存器，存放在接收的数据变量
        Serial_RxFlag = 1;                              //置接收标志位变量为1
        USART_ClearITPendingBit(USART1,USART_IT_RXNE);  //清除USART1的RXNE标志位
    }
}
