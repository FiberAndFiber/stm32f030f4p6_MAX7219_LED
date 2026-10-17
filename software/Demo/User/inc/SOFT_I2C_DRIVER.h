#ifndef __I2Cdriver_H
#define __I2Cdriver_H

void SOFTWARE_I2C_Init(void);                   //initialise
void SOFTWARE_I2C_Start(void);                  //Start
void SOFTWARE_I2C_End(void);                    //End
void SOFTWARE_I2C_SendByte(uint8_t Byte);       //SendByte
uint8_t SOFTWARE_I2C_ReciveByte(void);              //ReciveByte
void SOFTWARE_I2C_SendAnswer(uint8_t AnswerBit);//SendAnswer
uint8_t SOFTWARE_I2C_ReciveAnswer(void);            //ReciveAnswer

uint8_t I2C_ReadReg(uint8_t addr, uint8_t reg);

#endif
