#include "stm32f0xx.h"                  // Device header

#include "stm32f0xx_rcc.h"
#include "stm32f0xx_gpio.h"

#include "Delay.h"

/*
CS：PA6
SCK：PA5

MOSI：PA7
*/

void SOFE_SPI_DRIVER_CS(uint8_t BitValue)
{
    GPIO_WriteBit(GPIOA,GPIO_Pin_6,(BitAction)BitValue);
}

void SOFE_SPI_DRIVER_SCK(uint8_t BitValue)
{
    GPIO_WriteBit(GPIOA,GPIO_Pin_5,(BitAction)BitValue);
}

void SOFE_SPI_DRIVER_MOSI(uint8_t BitValue)
{
    GPIO_WriteBit(GPIOA,GPIO_Pin_7,(BitAction)BitValue);
}


void SOFE_SPI_DRIVER_INIT()
{
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA, ENABLE);

    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_5 | GPIO_Pin_7;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;		//输出
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;	//推挽
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA,&GPIO_InitStructure);

    SOFE_SPI_DRIVER_CS(1);
    SOFE_SPI_DRIVER_SCK(0);
}

void SOFE_SPI_DRIVER_START()
{
    SOFE_SPI_DRIVER_CS(0);
}

void SOFE_SPI_DRIVER_END()
{
    SOFE_SPI_DRIVER_CS(1);
}

void SOFE_SPI_DRIVER_SendByte(uint8_t ByteSend)
{
    uint8_t i;
    for (i = 0; i < 8; i++)
    {
        SOFE_SPI_DRIVER_MOSI(!!(ByteSend & (0x80 >> i)));
			  Delay_us(1);
        SOFE_SPI_DRIVER_SCK(1);
			  Delay_us(1);
        SOFE_SPI_DRIVER_SCK(0);
		  	Delay_us(1);
    }
}
