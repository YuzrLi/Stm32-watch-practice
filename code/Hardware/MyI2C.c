#include "stm32f10x.h"                  // Device header
#include "Delay.h"

/* ── Pin driver layer ─────────────────────────────────────────────────── */

/**
  * Function : Set SCL pin level
  * Arguments: BitValue — level to write (0 or 1), passed from protocol layer
  * Returns  : none
  * Note     : User must implement: drive SCL low when BitValue=0, high when BitValue=1
  */
void MyI2C_W_SCL(uint8_t BitValue)
{
	GPIO_WriteBit(GPIOB, GPIO_Pin_10, (BitAction)BitValue);		// Set SCL level according to BitValue
	Delay_us(10);												// 10us delay to keep timing within spec
}

/**
  * Function : Set SDA pin level
  * Arguments: BitValue — level to write (0 or 1), passed from protocol layer
  * Returns  : none
  * Note     : User must implement: drive SDA low when BitValue=0, high when BitValue=1
  */
void MyI2C_W_SDA(uint8_t BitValue)
{
	GPIO_WriteBit(GPIOB, GPIO_Pin_11, (BitAction)BitValue);		// Set SDA level (BitValue must be strictly 0 or 1)
	Delay_us(10);												// 10us delay
}

/**
  * Function : Read SDA pin level
  * Arguments: none
  * Returns  : Current SDA level (0 or 1)
  * Note     : User must implement: return 0 when SDA is low, return 1 when SDA is high
  */
uint8_t MyI2C_R_SDA(void)
{
	uint8_t BitValue;
	BitValue = GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_11);		// Read SDA level
	Delay_us(10);												// 10us delay
	return BitValue;											// Return SDA level
}

/**
  * Function : I2C GPIO initialization
  * Arguments: none
  * Returns  : none
  * Note     : User must implement: configure SCL and SDA pins as open-drain outputs
  */
void MyI2C_Init(void)
{
	/* Enable clock */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);	// Enable GPIOB clock

	/* GPIO initialization */
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10 | GPIO_Pin_11;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);					// Configure PB10 and PB11 as open-drain outputs

	/* Set default levels */
	GPIO_SetBits(GPIOB, GPIO_Pin_10 | GPIO_Pin_11);			// Default both pins high (release bus)
}

/* ── Protocol layer ───────────────────────────────────────────────────── */

/**
  * Function : I2C START condition
  * Arguments: none
  * Returns  : none
  */
void MyI2C_Start(void)
{
	MyI2C_W_SDA(1);							// Release SDA — ensure SDA is high
	MyI2C_W_SCL(1);							// Release SCL — ensure SCL is high
	MyI2C_W_SDA(0);							// Pull SDA low while SCL is high — generates START condition
	MyI2C_W_SCL(0);							// Pull SCL low to claim the bus and prepare for data
}

/**
  * Function : I2C STOP condition
  * Arguments: none
  * Returns  : none
  */
void MyI2C_Stop(void)
{
	MyI2C_W_SDA(0);							// Ensure SDA is low
	MyI2C_W_SCL(1);							// Release SCL — SCL goes high
	MyI2C_W_SDA(1);							// Release SDA while SCL is high — generates STOP condition
}

/**
  * Function : Send one byte over I2C
  * Arguments: Byte — byte to send, range: 0x00~0xFF
  * Returns  : none
  */
void MyI2C_SendByte(uint8_t Byte)
{
	uint8_t i;
	for (i = 0; i < 8; i ++)				// Loop 8 times, MSB first
	{
		/* !! performs double logical NOT: converts any non-zero to 1, zero stays 0 */
		MyI2C_W_SDA(!!(Byte & (0x80 >> i)));// Extract bit i and write to SDA
		MyI2C_W_SCL(1);						// Release SCL — slave reads SDA on rising edge
		MyI2C_W_SCL(0);						// Pull SCL low — master prepares next bit
	}
}

/**
  * Function : Receive one byte over I2C
  * Arguments: none
  * Returns  : Received byte, range: 0x00~0xFF
  */
uint8_t MyI2C_ReceiveByte(void)
{
	uint8_t i, Byte = 0x00;					// Initialize to 0x00 (bits are OR'd in below)
	MyI2C_W_SDA(1);							// Release SDA before reading to avoid interfering with slave
	for (i = 0; i < 8; i ++)				// Loop 8 times, MSB first
	{
		MyI2C_W_SCL(1);						// Release SCL — master reads SDA on high level
		if (MyI2C_R_SDA()){Byte |= (0x80 >> i);}	// If SDA=1, set corresponding bit in Byte
													// If SDA=0, bit stays 0 (default from initialization)
		MyI2C_W_SCL(0);						// Pull SCL low — slave writes next bit
	}
	return Byte;							// Return received byte
}

/**
  * Function : Send ACK/NACK bit
  * Arguments: AckBit — 0 = ACK, 1 = NACK
  * Returns  : none
  */
void MyI2C_SendAck(uint8_t AckBit)
{
	MyI2C_W_SDA(AckBit);					// Place ACK/NACK on SDA
	MyI2C_W_SCL(1);							// Release SCL — slave reads ACK on high level
	MyI2C_W_SCL(0);							// Pull SCL low — begin next sequence
}

/**
  * Function : Receive ACK/NACK bit
  * Arguments: none
  * Returns  : AckBit — 0 = ACK (slave acknowledged), 1 = NACK (slave did not acknowledge)
  */
uint8_t MyI2C_ReceiveAck(void)
{
	uint8_t AckBit;							// Variable to store ACK bit
	MyI2C_W_SDA(1);							// Release SDA before reading to avoid interfering with slave
	MyI2C_W_SCL(1);							// Release SCL — master reads SDA on high level
	AckBit = MyI2C_R_SDA();					// Read and store ACK bit
	MyI2C_W_SCL(0);							// Pull SCL low — begin next sequence
	return AckBit;							// Return ACK bit
}
