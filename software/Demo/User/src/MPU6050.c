#include "stm32f0xx.h"                  // Device header

#include "SOFT_I2C_DRIVER.h"

#define MPU6050_ADDRESS    0xD0

/*寄存器地址*/
#define	MPU6050_SMPLRT_DIV		0x19    //采用率分频
#define	MPU6050_CONFIG			  0x1A    //数字低通滤波器
#define	MPU6050_GYRO_CONFIG		0x1B    //陀螺仪配寄存器
#define	MPU6050_ACCEL_CONFIG	0x1C    //加速度计寄存器

#define	MPU6050_ACCEL_XOUT_H	0x3B
#define	MPU6050_ACCEL_XOUT_L	0x3C
#define	MPU6050_ACCEL_YOUT_H	0x3D
#define	MPU6050_ACCEL_YOUT_L	0x3E
#define	MPU6050_ACCEL_ZOUT_H	0x3F
#define	MPU6050_ACCEL_ZOUT_L	0x40
#define	MPU6050_TEMP_OUT_H		0x41
#define	MPU6050_TEMP_OUT_L		0x42
#define	MPU6050_GYRO_XOUT_H		0x43
#define	MPU6050_GYRO_XOUT_L		0x44
#define	MPU6050_GYRO_YOUT_H		0x45
#define	MPU6050_GYRO_YOUT_L		0x46
#define	MPU6050_GYRO_ZOUT_H		0x47
#define	MPU6050_GYRO_ZOUT_L		0x48

#define	MPU6050_PWR_MGMT_1		0x6B    //电源管理寄存器1
#define	MPU6050_PWR_MGMT_2		0x6C    //电源管理寄存器2
#define	MPU6050_WHO_AM_I		0x75


void MPU6050_WriteReg(uint8_t RegAddress , uint8_t Data)
{
    SOFTWARE_I2C_Start();
    SOFTWARE_I2C_SendByte(MPU6050_ADDRESS);
    SOFTWARE_I2C_ReciveAnswer();
    SOFTWARE_I2C_SendByte(RegAddress);
    SOFTWARE_I2C_ReciveAnswer();
    SOFTWARE_I2C_SendByte(Data);
    SOFTWARE_I2C_ReciveAnswer();
    SOFTWARE_I2C_End();
}

uint8_t MPU6050_ReadReg(uint8_t RegAddress)
{   
    uint8_t Data;

    SOFTWARE_I2C_Start();
    SOFTWARE_I2C_SendByte(MPU6050_ADDRESS);
    SOFTWARE_I2C_ReciveAnswer();
    SOFTWARE_I2C_SendByte(RegAddress);
    SOFTWARE_I2C_ReciveAnswer();
    
    SOFTWARE_I2C_Start();
    SOFTWARE_I2C_SendByte(MPU6050_ADDRESS | 0x01);
    SOFTWARE_I2C_ReciveAnswer();
    Data = SOFTWARE_I2C_ReciveByte();  
    SOFTWARE_I2C_SendAnswer(1);
    SOFTWARE_I2C_End();

    return Data;
}

void MPU6050_Init()
{
    SOFTWARE_I2C_Init();
    MPU6050_WriteReg(MPU6050_PWR_MGMT_1, 0x01);
    MPU6050_WriteReg(MPU6050_PWR_MGMT_2, 0x00);

    MPU6050_WriteReg(MPU6050_SMPLRT_DIV, 0x09);
    MPU6050_WriteReg(MPU6050_CONFIG, 0x02);
    MPU6050_WriteReg(MPU6050_GYRO_CONFIG, 0x18);
    MPU6050_WriteReg(MPU6050_ACCEL_CONFIG, 0x18);
}

void MPU6050_GetData(int16_t *AccX,int16_t *AccY,int16_t *AccZ,
                    int16_t *GccX,int16_t *GccY,int16_t *GccZ)
{
    uint16_t DataH,DataL;

    /**/
    DataH = MPU6050_ReadReg(MPU6050_ACCEL_XOUT_H);
    DataL = MPU6050_ReadReg(MPU6050_ACCEL_XOUT_L);
    *AccX = (DataH << 8) | DataL;

    DataH = MPU6050_ReadReg(MPU6050_ACCEL_YOUT_H);
    DataL = MPU6050_ReadReg(MPU6050_ACCEL_YOUT_L);
    *AccY = (DataH << 8) | DataL;

    DataH = MPU6050_ReadReg(MPU6050_ACCEL_ZOUT_H);
    DataL = MPU6050_ReadReg(MPU6050_ACCEL_ZOUT_L);
    *AccZ = (DataH << 8) | DataL;

    /**/
    DataH = MPU6050_ReadReg(MPU6050_GYRO_XOUT_H);
    DataL = MPU6050_ReadReg(MPU6050_GYRO_XOUT_L);
    *GccX = (DataH << 8) | DataL;

    DataH = MPU6050_ReadReg(MPU6050_GYRO_YOUT_H);
    DataL = MPU6050_ReadReg(MPU6050_GYRO_YOUT_L);
    *GccY = (DataH << 8) | DataL;

    DataH = MPU6050_ReadReg(MPU6050_GYRO_ZOUT_H);
    DataL = MPU6050_ReadReg(MPU6050_GYRO_ZOUT_L);
    *GccZ = (DataH << 8) | DataL;
}
