#ifndef __AT24C02_H
#define __AT24C02_H

void AT24C02_WriteReg(uint8_t RegAddress , uint8_t Data);
uint8_t AT24C02_ReadReg(uint8_t RegAddress);

#endif 