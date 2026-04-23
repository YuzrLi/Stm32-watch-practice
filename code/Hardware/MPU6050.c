#include "stm32f10x.h"                  // Device header
#include "MyI2C.h"
#include "MPU6050_Reg.h"

#define MPU6050_ADDRESS		0xD0		// MPU6050 I2C slave address

/**
  * Function : Write MPU6050 register
  * Arguments: RegAddress — register address (see MPU6050 register map)
  *            Data       — data to write, range: 0x00~0xFF
  * Returns  : none
  */
void MPU6050_WriteReg(uint8_t RegAddress, uint8_t Data)
{
	MyI2C_Start();						// I2C START condition
	MyI2C_SendByte(MPU6050_ADDRESS);	// Send slave address with R/W=0 (write)
	MyI2C_ReceiveAck();					// Receive ACK
	MyI2C_SendByte(RegAddress);			// Send register address
	MyI2C_ReceiveAck();					// Receive ACK
	MyI2C_SendByte(Data);				// Send data byte
	MyI2C_ReceiveAck();					// Receive ACK
	MyI2C_Stop();						// I2C STOP condition
}

/**
  * Function : Read MPU6050 register
  * Arguments: RegAddress — register address (see MPU6050 register map)
  * Returns  : register value, range: 0x00~0xFF
  */
uint8_t MPU6050_ReadReg(uint8_t RegAddress)
{
	uint8_t Data;

	MyI2C_Start();						// I2C START condition
	MyI2C_SendByte(MPU6050_ADDRESS);	// Send slave address with R/W=0 (write — set register pointer)
	MyI2C_ReceiveAck();					// Receive ACK
	MyI2C_SendByte(RegAddress);			// Send register address
	MyI2C_ReceiveAck();					// Receive ACK

	MyI2C_Start();						// Repeated START condition
	MyI2C_SendByte(MPU6050_ADDRESS | 0x01);	// Send slave address with R/W=1 (read)
	MyI2C_ReceiveAck();					// Receive ACK
	Data = MyI2C_ReceiveByte();			// Read register data
	MyI2C_SendAck(1);					// Send NACK to signal end of read
	MyI2C_Stop();						// I2C STOP condition

	return Data;
}

/**
  * Function : MPU6050 initialization
  * Arguments: none
  * Returns  : none
  */
void MPU6050_Init(void)
{
	MyI2C_Init();									// Initialize the underlying I2C bus first

	/* Configure key MPU6050 registers (refer to MPU6050 register map for full details) */
	MPU6050_WriteReg(MPU6050_PWR_MGMT_1, 0x01);	// Power management 1: disable sleep, use X-axis gyro as clock source
	MPU6050_WriteReg(MPU6050_PWR_MGMT_2, 0x00);	// Power management 2: keep default (all axes active)
	MPU6050_WriteReg(MPU6050_SMPLRT_DIV, 0x04);	// Sample rate divider: set sampling rate
	MPU6050_WriteReg(MPU6050_CONFIG, 0x06);			// Configuration: set DLPF (digital low-pass filter)
	MPU6050_WriteReg(MPU6050_GYRO_CONFIG, 0x18);	// Gyroscope config: full scale = ±2000°/s
	MPU6050_WriteReg(MPU6050_ACCEL_CONFIG, 0x18);	// Accelerometer config: full scale = ±16g
}

/**
  * Function : Get MPU6050 device ID
  * Arguments: none
  * Returns  : MPU6050 WHO_AM_I register value
  */
uint8_t MPU6050_GetID(void)
{
	return MPU6050_ReadReg(MPU6050_WHO_AM_I);		// Return WHO_AM_I register value
}

/**
  * Function : Get MPU6050 sensor data
  * Arguments: AccX AccY AccZ  — accelerometer X/Y/Z output (via pointer), range: -32768~32767
  *            GyroX GyroY GyroZ — gyroscope X/Y/Z output (via pointer), range: -32768~32767
  * Returns  : none
  */
void MPU6050_GetData(int16_t *AccX, int16_t *AccY, int16_t *AccZ,
						int16_t *GyroX, int16_t *GyroY, int16_t *GyroZ)
{
	uint8_t DataH, DataL;								// High and low bytes of each raw reading

	DataH = MPU6050_ReadReg(MPU6050_ACCEL_XOUT_H);		// Accelerometer X high byte
	DataL = MPU6050_ReadReg(MPU6050_ACCEL_XOUT_L);		// Accelerometer X low byte
	*AccX = (DataH << 8) | DataL;						// Combine into 16-bit signed value

	DataH = MPU6050_ReadReg(MPU6050_ACCEL_YOUT_H);		// Accelerometer Y high byte
	DataL = MPU6050_ReadReg(MPU6050_ACCEL_YOUT_L);		// Accelerometer Y low byte
	*AccY = (DataH << 8) | DataL;

	DataH = MPU6050_ReadReg(MPU6050_ACCEL_ZOUT_H);		// Accelerometer Z high byte
	DataL = MPU6050_ReadReg(MPU6050_ACCEL_ZOUT_L);		// Accelerometer Z low byte
	*AccZ = (DataH << 8) | DataL;

	DataH = MPU6050_ReadReg(MPU6050_GYRO_XOUT_H);		// Gyroscope X high byte
	DataL = MPU6050_ReadReg(MPU6050_GYRO_XOUT_L);		// Gyroscope X low byte
	*GyroX = (DataH << 8) | DataL;

	DataH = MPU6050_ReadReg(MPU6050_GYRO_YOUT_H);		// Gyroscope Y high byte
	DataL = MPU6050_ReadReg(MPU6050_GYRO_YOUT_L);		// Gyroscope Y low byte
	*GyroY = (DataH << 8) | DataL;

	DataH = MPU6050_ReadReg(MPU6050_GYRO_ZOUT_H);		// Gyroscope Z high byte
	DataL = MPU6050_ReadReg(MPU6050_GYRO_ZOUT_L);		// Gyroscope Z low byte
	*GyroZ = (DataH << 8) | DataL;
}
