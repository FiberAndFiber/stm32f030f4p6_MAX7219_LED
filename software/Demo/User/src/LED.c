#include "stm32f0xx.h"                  // Device header

#include "stm32f0xx_rcc.h"
#include "stm32f0xx_gpio.h"

#include "Delay.h"

#define LED_ON      1
#define LED_OFF     0

void LED_Init(void)
{
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA,ENABLE);

    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

    GPIO_Init(GPIOA,&GPIO_InitStructure);
}

/*
LED_ON      1
LED_OFF     0
*/
void LED_Control(uint8_t value)
{
    if (value == LED_ON)
        GPIO_SetBits(GPIOA, GPIO_Pin_0);
    else
        GPIO_ResetBits(GPIOA, GPIO_Pin_0);
}
