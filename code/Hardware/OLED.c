
/* OLED Driver — SSD1306 128x64, I2C on PB8 (SCL) / PB9 (SDA), STM32F103 */

#include "stm32f10x.h"
#include "OLED.h"
#include <string.h>
#include <math.h>
#include <stdio.h>
#include <stdarg.h>

/**
  * Data Format and Memory Layout
  *   Each byte represents 8 pixels stacked vertically
  * Bit order: LSB at top (Y=0), MSB at bottom (Y=7)
  *
  *      B0 B0                  B0 B0
  *      B1 B1                  B1 B1
  *      B2 B2                  B2 B2
  *      B3 B3  ------------->  B3 B3 --
  *      B4 B4                  B4 B4  |
  *      B5 B5                  B5 B5  |
  *      B6 B6                  B6 B6  |
  *      B7 B7                  B7 B7  |
  *                                    |
  *  -----------------------------------
  *  |
  *  |   B0 B0                  B0 B0
  *  |   B1 B1                  B1 B1
  *  |   B2 B2                  B2 B2
  *  --> B3 B3  ------------->  B3 B3
  *      B4 B4                  B4 B4
  *      B5 B5                  B5 B5
  *      B6 B6                  B6 B6
  *      B7 B7                  B7 B7
  *
  *    Coordinate System
  *    Origin at (0, 0) top-left corner
  *       X axis ranges from 0~127 (left to right)
  *       Y axis ranges from 0~63 (top to bottom)
  *
  *      0             X             127
  *      .------------------------------->
  *    0 |
  *      |
  *      |
  *      |
  *  Y   |
  *      |
  *      |
  *      |
  *   63 |
  *      v
  *
  */


/*Global Variables ********************/

/**
  * OLED display buffer
  *    Maintains the 128x64 pixel display data
  *    Must call OLED_Update or OLED_UpdateArea to push to hardware
  */
uint8_t OLED_DisplayBuf[8][128];

/**Global Variables **/


/*GPIO Pin Control ********************/

/**
  * Set OLED I2C SCL pin output level
  *    Sets the SCL line to high (1) or low (0)
  *    Description
  * Note: SCL is open-drain output on PB8, driven by OLED_W_SCL
  *       A value of 0 pulls SCL low; a value of 1 releases it (pull-up)
  *                     0: SCL driven low               1: SCL released (pulled high)
  */
void OLED_W_SCL(uint8_t BitValue)
{
	/*    Set SCL pin output level based on BitValue*/
	GPIO_WriteBit(GPIOB, GPIO_Pin_8, (BitAction)BitValue);

	/*    Delay for I2C timing (setup/hold time)*/
	//...
}

/**
  * Set OLED I2C SDA pin output level
  *    Sets the SDA line to high (1) or low (0)
  *    Description
  * Note: SDA is open-drain output on PB9, driven by OLED_W_SDA
  *       A value of 0 pulls SDA low; a value of 1 releases it (pull-up)
  *                     0: SDA driven low               1: SDA released (pulled high)
  */
void OLED_W_SDA(uint8_t BitValue)
{
	/*    Set SDA pin output level based on BitValue*/
	GPIO_WriteBit(GPIOB, GPIO_Pin_9, (BitAction)BitValue);

	/*    Delay for I2C timing (setup/hold time)*/
	//...
}

/**
  * Initialize OLED GPIO pins
  *    Initialize I2C pins for OLED communication
  *    Description
  * Note: Initializes SCL (PB8) and SDA (PB9) as open-drain outputs
  */
void OLED_GPIO_Init(void)
{
	uint32_t i, j;

	/*    Initial delay to allow power stabilization of OLED*/
	for (i = 0; i < 1000; i ++)
	{
		for (j = 0; j < 1000; j ++);
	}

	/*    Enable GPIO clock for port B*/
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

	GPIO_InitTypeDef GPIO_InitStructure;
 	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
 	GPIO_Init(GPIOB, &GPIO_InitStructure);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
 	GPIO_Init(GPIOB, &GPIO_InitStructure);

	/*    Set SCL and SDA to idle state (high)*/
	OLED_W_SCL(1);
	OLED_W_SDA(1);
}

/**GPIO Pin Control **/


/*I2C Communication ********************/

/**
  * Generate I2C START condition
  *    Releases SDA while SCL is high to signal START
  *    Description
  */
void OLED_I2C_Start(void)
{
	OLED_W_SDA(1);		//  Set SDA high, ensure SDA is released
	OLED_W_SCL(1);		//  Set SCL high, ensure SCL is released
	OLED_W_SDA(0);		//  Pull SDA low while SCL stays high (START condition)
	OLED_W_SCL(0);		//  Pull SCL low to prepare for data transmission
}

/**
  * Generate I2C STOP condition
  *    Releases SDA while SCL is high to signal STOP
  *    Description
  */
void OLED_I2C_Stop(void)
{
	OLED_W_SDA(0);		//    Pull SDA low, ensure SDA is driven
	OLED_W_SCL(1);		//    Set SCL high, SCL clock pulse for ACK
	OLED_W_SDA(1);		//    Release SDA to high while SCL is high (STOP condition)
}

/**
  * Send one byte over I2C bus
  *    Transmits 8 bits MSB-first; slave acknowledges with clock pulse
  *    Description
  */
void OLED_I2C_SendByte(uint8_t Byte)
{
	uint8_t i;

	/*    Send 8 bits, starting from most significant bit*/
	for (i = 0; i < 8; i++)
	{
		/*    Extract bit from Byte and place on SDA line*/
		/*    MSB first: bit 7, 6, 5... down to bit 0*/
		OLED_W_SDA(!!(Byte & (0x80 >> i)));
		OLED_W_SCL(1);	//  Clock pulse: SCL high to signal data valid
		OLED_W_SCL(0);	//    Lower SCL to prepare for next bit
	}

	OLED_W_SCL(1);		//    Clock pulse for slave ACK
	OLED_W_SCL(0);
}

/**
  * Write command byte to OLED via I2C
  *    Sends single command byte; slave acknowledges
  *    Description
  */
void OLED_WriteCommand(uint8_t Command)
{
	OLED_I2C_Start();				//I2C START condition
	OLED_I2C_SendByte(0x78);		//    Slave address (OLED I2C address 0x78)
	OLED_I2C_SendByte(0x00);		//    Control byte 0x00 (command mode)
	OLED_I2C_SendByte(Command);		//  Send the command byte
	OLED_I2C_Stop();				//I2C STOP condition
}

/**
  * Write data bytes to OLED via I2C
  *    Sends multiple data bytes to display buffer
  *    Count: number of bytes to send
  *    Description
  */
void OLED_WriteData(uint8_t *Data, uint8_t Count)
{
	uint8_t i;

	OLED_I2C_Start();				//I2C START condition
	OLED_I2C_SendByte(0x78);		//    Slave address (OLED I2C address 0x78)
	OLED_I2C_SendByte(0x40);		//    Control byte 0x40 (data mode)
	/*    Send Count bytes from Data buffer*/
	for (i = 0; i < Count; i ++)
	{
		OLED_I2C_SendByte(Data[i]);	//   Send one data byte
	}
	OLED_I2C_Stop();				//I2C STOP condition
}

/**I2C Communication **/


/*Initialization ********************/

/**
  * Initialize OLED display
  *    Configures GPIO and SSD1306 controller for 128x64 display
  *    Description
  * Note: Sends standard SSD1306 init sequence to enable display
  */
void OLED_Init(void)
{
	OLED_GPIO_Init();			//    Initialize GPIO pins for I2C

	/*    SSD1306 controller initialization sequence*/
	OLED_WriteCommand(0xAE);	//      Display OFF; use 0xAE to turn off, 0xAF to turn on

	OLED_WriteCommand(0xD5);	//      Set display clock ratio/frequency
	OLED_WriteCommand(0x80);	//0x00~0xFF

	OLED_WriteCommand(0xA8);	//      Set MUX ratio (number of output lines)
	OLED_WriteCommand(0x3F);	//0x0E~0x3F

	OLED_WriteCommand(0xD3);	//      Set display offset (vertical shift)
	OLED_WriteCommand(0x00);	//0x00~0x7F

	OLED_WriteCommand(0x40);	//      Set display start line (first visible row)

	OLED_WriteCommand(0xA1);	//      Set segment remap: 0xA1 normal, 0xA0 for reverse

	OLED_WriteCommand(0xC8);	//      Set COM output scan direction: 0xC8 normal, 0xC0 reverse

	OLED_WriteCommand(0xDA);	//      Set COM pins hardware config
	OLED_WriteCommand(0x12);

	OLED_WriteCommand(0x81);	//      Set contrast level
	OLED_WriteCommand(0xCF);	//0x00~0xFF

	OLED_WriteCommand(0xD9);	//      Set pre-charge period
	OLED_WriteCommand(0xF1);

	OLED_WriteCommand(0xDB);	//      Set VCOMH deselect voltage
	OLED_WriteCommand(0x30);

	OLED_WriteCommand(0xA4);	//      Set display mode: normal output / RAM content

	OLED_WriteCommand(0xA6);	//      Set display mode: normal/inverted; 0xA6 normal, 0xA7 inverted

	OLED_WriteCommand(0x8D);	//      Enable internal charge pump
	OLED_WriteCommand(0x14);

	OLED_WriteCommand(0xAF);	//      Display ON

	OLED_Clear();				//    Clear display buffer
	OLED_Update();				//      Transfer buffer to OLED hardware
}

/**
  * Set cursor position in OLED display buffer
  *    Page: row index (0~7, each page is 8 pixels tall)
  *    X: column index (0~127)
  *    Description
  * Note: OLED memory is arranged in pages (rows of 8 pixels); Y address is implicit in page
  */
void OLED_SetCursor(uint8_t Page, uint8_t X)
{
	/*    Note: 0.96" OLED (SSD1306) uses X offset 0; no modification needed*/
	/*    Note: 1.3" OLED (SH1106) requires X += 2 for alignment*/

	/*    Set page (row) address for cursor*/
	OLED_WriteCommand(0xB0 | Page);					//    Set page address
	OLED_WriteCommand(0x10 | ((X & 0xF0) >> 4));	//    Set X column upper 4 bits
	OLED_WriteCommand(0x00 | (X & 0x0F));			//    Set X column lower 4 bits
}

/**Initialization **/


/*Math Utilities ********************/

/*Math utility functions for drawing operations*/

/**
  * Calculate power: X^Y
  *    X: base value
  *    Y: exponent value
  *    Returns: X to the power of Y
  */
uint32_t OLED_Pow(uint32_t X, uint32_t Y)
{
	uint32_t Result = 1;	//    Initialize result to 1
	while (Y --)			//    Loop Y times
	{
		Result *= X;		//    Multiply result by X each iteration
	}
	return Result;
}

/**
  * Point-in-polygon test using ray casting algorithm
  *    nvert: number of vertices
  *    vertx, verty: vertex coordinate arrays (x, y coordinates)
  *    testx, testy: point to test (X, Y coordinates)
  *    Returns: 1 if point is inside polygon, 0 if outside
  */
uint8_t OLED_pnpoly(uint8_t nvert, int16_t *vertx, int16_t *verty, int16_t testx, int16_t testy)
{
	int16_t i, j, c = 0;

	/*    Algorithm by W. Randolph Franklin*/
	/* W. Randolph Franklin point-in-polygon algorithm */
	for (i = 0, j = nvert - 1; i < nvert; j = i++)
	{
		if (((verty[i] > testy) != (verty[j] > testy)) &&
			(testx < (vertx[j] - vertx[i]) * (testy - verty[i]) / (verty[j] - verty[i]) + vertx[i]))
		{
			c = !c;
		}
	}
	return c;
}

/**
  * Check if angle is within given angular range
  *    X Y: relative coordinates to test point
  *    StartAngle EndAngle: angular bounds in degrees (-180~180)
  *    Angle 0 is rightward, 180 is leftward, -180 is also leftward; ranges wrap correctly
  *    Returns: 1 if angle is within range, 0 if outside
  */
uint8_t OLED_IsInAngle(int16_t X, int16_t Y, int16_t StartAngle, int16_t EndAngle)
{
	int16_t PointAngle;
	PointAngle = atan2(Y, X) / 3.14 * 180;	//    Convert polar angle to degrees
	if (StartAngle < EndAngle)	//    Normal range (no wrap)
	{
		/*    Angle is in range if within [StartAngle, EndAngle]*/
		if (PointAngle >= StartAngle && PointAngle <= EndAngle)
		{
			return 1;
		}
	}
	else			//    Wrapped range (crosses -180/180 boundary)
	{
		/*    Angle is in range if >= StartAngle OR <= EndAngle*/
		if (PointAngle >= StartAngle || PointAngle <= EndAngle)
		{
			return 1;
		}
	}
	return 0;		//    Angle is outside the specified range
}

/**Math Utilities **/


/*Buffer Operations ********************/

/**
  * Update entire OLED display from buffer
  *    Transfers all 128x64 pixels from OLED_DisplayBuf to hardware
  *    Description
  * Note: Slow operation; for partial updates use OLED_UpdateArea
  *       Call after modifying OLED_DisplayBuf to push changes to hardware
  *       Overwrites entire OLED display with buffer contents
  */
void OLED_Update(void)
{
	uint8_t j;
	/*    Iterate through all 8 pages*/
	for (j = 0; j < 8; j ++)
	{
		/*    Set cursor to start of this page (row)*/
		OLED_SetCursor(j, 0);
		/*    Send 128 bytes (full width) of data for this page to OLED*/
		OLED_WriteData(OLED_DisplayBuf[j], 128);
	}
}

/**
  * Update portion of OLED display from buffer
  *    X: starting column (supports -32768~32767 range, visible 0~127)
  *    Y: starting row (supports -32768~32767 range, visible 0~63)
  *    Width: update width in pixels (0~128)
  *    Height: update height in pixels (0~64)
  *    Description
  * Note: Handles coordinates spanning multiple pages (8-pixel rows)
  *       Y coordinates may straddle page boundaries
  * Note: Slow operation; only update necessary regions
  *       Call after modifying part of OLED_DisplayBuf
  *       Overwrites specified rectangular region with buffer contents
  */
void OLED_UpdateArea(int16_t X, int16_t Y, uint8_t Width, uint8_t Height)
{
	int16_t j;
	int16_t Page, Page1;

	/*    Calculate start and end pages (8-pixel rows)*/
	/*(Y + Height - 1) / 8 + 1 equals (Y + Height) / 8 with rounding up*/
	Page = Y / 8;
	Page1 = (Y + Height - 1) / 8 + 1;
	if (Y < 0)
	{
		Page -= 1;
		Page1 -= 1;
	}

	/*    Update each affected page row*/
	for (j = Page; j < Page1; j ++)
	{
		if (X >= 0 && X <= 127 && j >= 0 && j <= 7)		//    Boundary check
		{
			/*    Set cursor to start of this region*/
			OLED_SetCursor(j, X);
			/*    Send Width bytes for this partial row*/
			OLED_WriteData(&OLED_DisplayBuf[j][X], Width);
		}
	}
}

/**
  * Clear entire OLED display buffer
  *    Fills OLED_DisplayBuf with zeros (all pixels off)
  *    Description
  * Note: Must call OLED_Update or OLED_UpdateArea to apply to hardware
  */
void OLED_Clear(void)
{
	uint8_t i, j;
	for (j = 0; j < 8; j ++)				//    Clear all 8 pages
	{
		for (i = 0; i < 128; i ++)			//    Clear all 128 columns
		{
			OLED_DisplayBuf[j][i] = 0x00;	//    Set pixel byte to off
		}
	}
}

/**
  * Clear rectangular region in OLED display buffer
  *    X: starting column (supports -32768~32767 range, visible 0~127)
  *    Y: starting row (supports -32768~32767 range, visible 0~63)
  *    Width: clear width in pixels (0~128)
  *    Height: clear height in pixels (0~64)
  *    Description
  * Note: Must call OLED_Update or OLED_UpdateArea to apply to hardware
  */
void OLED_ClearArea(int16_t X, int16_t Y, uint8_t Width, uint8_t Height)
{
	int16_t i, j;

	for (j = Y; j < Y + Height; j ++)		//    Clear each row
	{
		for (i = X; i < X + Width; i ++)	//    Clear each column
		{
			if (i >= 0 && i <= 127 && j >=0 && j <= 63)				//    Boundary check
			{
				OLED_DisplayBuf[j / 8][i] &= ~(0x01 << (j % 8));	//    Clear pixel bit
			}
		}
	}
}

/**
  * Invert entire OLED display buffer (XOR with 0xFF)
  *    All pixels flip: on becomes off, off becomes on
  *    Description
  * Note: Must call OLED_Update or OLED_UpdateArea to apply to hardware
  */
void OLED_Reverse(void)
{
	uint8_t i, j;
	for (j = 0; j < 8; j ++)				//    Process all 8 pages
	{
		for (i = 0; i < 128; i ++)			//    Process all 128 columns
		{
			OLED_DisplayBuf[j][i] ^= 0xFF;	//    Invert all pixels in this byte
		}
	}
}

/**
  * Invert rectangular region in OLED display buffer
  *    X: starting column (supports -32768~32767 range, visible 0~127)
  *    Y: starting row (supports -32768~32767 range, visible 0~63)
  *    Width: invert width in pixels (0~128)
  *    Height: invert height in pixels (0~64)
  *    Description
  * Note: Must call OLED_Update or OLED_UpdateArea to apply to hardware
  */
void OLED_ReverseArea(int16_t X, int16_t Y, uint8_t Width, uint8_t Height)
{
	int16_t i, j;

	for (j = Y; j < Y + Height; j ++)		//    Process each row
	{
		for (i = X; i < X + Width; i ++)	//    Process each column
		{
			if (i >= 0 && i <= 127 && j >=0 && j <= 63)			//    Boundary check
			{
				OLED_DisplayBuf[j / 8][i] ^= 0x01 << (j % 8);	//    Invert pixel bit
			}
		}
	}
}

/**
  * Display single character at position
  *    X: column coordinate (supports -32768~32767 range, visible 0~127)
  *    Y: row coordinate (supports -32768~32767 range, visible 0~63)
  *    Char: character to display (ASCII code)
  *    FontSize: font selection
  *      Available: OLED_8X16 (8 width x 16 height)
  *                 OLED_6X8  (6 width x 8 height)
  *                 OLED_12X24 (12 width x 24 height)
  *    Description
  * Note: Must call OLED_Update or OLED_UpdateArea to apply to hardware
  */
void OLED_ShowChar(int16_t X, int16_t Y, char Char, uint8_t FontSize)
{
	if (FontSize == OLED_8X16)		//    Use 8x16 font size
	{
		/*    Look up ASCII character in OLED_F8x16 array and display as 8x16 image*/
		OLED_ShowImage(X, Y, 8, 16, OLED_F8x16[Char - ' ']);
	}
	else if(FontSize == OLED_6X8)	//    Use 6x8 font size
	{
		/*    Look up ASCII character in OLED_F6x8 array and display as 6x8 image*/
		OLED_ShowImage(X, Y, 6, 8, OLED_F6x8[Char - ' ']);
	}

	else if(FontSize == OLED_12X24)	//    Use 12x24 font size
	{
		/*    Look up ASCII character in OLED_F12x24 array and display as 12x24 image*/
		OLED_ShowImage(X, Y, 12, 24, OLED_F12x24[Char - ' ']);
	}
}

/**
  * Display ASCII string at position (multi-byte characters show as '?')
  *    X: column coordinate (supports -32768~32767 range, visible 0~127)
  *    Y: row coordinate (supports -32768~32767 range, visible 0~63)
  *    String: pointer to null-terminated ASCII string
  *    FontSize: font selection
  *      Available: OLED_8X16 (8 width x 16 height)
  *                 OLED_6X8  (6 width x 8 height)
  *    Description
  * Note: Chinese font (OLED_CF16x16) requires external file OLED_Data.c
  *      Full multi-byte UTF8/GB2312 support requires font data not included
  *      Without font data, multi-byte characters display as '?'
  *         With OLED_8X16, multi-byte chars display as 16x16 blocks
  *         With OLED_6X8, multi-byte chars display as 6x8 blocks
  * Note: Must call OLED_Update or OLED_UpdateArea to apply to hardware
  */
void OLED_ShowString(int16_t X, int16_t Y, char *String, uint8_t FontSize)
{
	uint16_t i = 0;
	char SingleChar[5];
	uint8_t CharLength = 0;
	uint16_t XOffset = 0;

	while (String[i] != '\0')	//    Process each character in string
	{

#ifdef OLED_CHARSET_UTF8						//    UTF-8 character encoding enabled
		/*    Parse multi-byte UTF-8 sequences and store in SingleChar array*/
		/*    Handle UTF-8 encoding rules for 1-4 byte characters*/
		if ((String[i] & 0x80) == 0x00)			//    Single byte: 0xxxxxxx
		{
			CharLength = 1;						//    1 byte character
			SingleChar[0] = String[i ++];		//    Copy byte to SingleChar[0]
			SingleChar[1] = '\0';				//    Null-terminate SingleChar
		}
		else if ((String[i] & 0xE0) == 0xC0)	//    2-byte char: 110xxxxx
		{
			CharLength = 2;						//    2 byte character
			SingleChar[0] = String[i ++];		//    Copy first byte to SingleChar[0]
			if (String[i] == '\0') {break;}		//    Check for string end
			SingleChar[1] = String[i ++];		//    Copy second byte to SingleChar[1]
			SingleChar[2] = '\0';				//    Null-terminate SingleChar
		}
		else if ((String[i] & 0xF0) == 0xE0)	//    3-byte char: 1110xxxx
		{
			CharLength = 3;						//    3 byte character
			SingleChar[0] = String[i ++];
			if (String[i] == '\0') {break;}
			SingleChar[1] = String[i ++];
			if (String[i] == '\0') {break;}
			SingleChar[2] = String[i ++];
			SingleChar[3] = '\0';
		}
		else if ((String[i] & 0xF8) == 0xF0)	//    4-byte char: 11110xxx
		{
			CharLength = 4;						//    4 byte character
			SingleChar[0] = String[i ++];
			if (String[i] == '\0') {break;}
			SingleChar[1] = String[i ++];
			if (String[i] == '\0') {break;}
			SingleChar[2] = String[i ++];
			if (String[i] == '\0') {break;}
			SingleChar[3] = String[i ++];
			SingleChar[4] = '\0';
		}
		else
		{
			i ++;			//    Skip invalid byte and continue parsing
			continue;
		}
#endif

#ifdef OLED_CHARSET_GB2312						//    GB2312 character encoding enabled
		/*    Parse multi-byte GB2312 sequences and store in SingleChar array*/
		/*    Handle GB2312 encoding rules for 1-2 byte characters*/
		if ((String[i] & 0x80) == 0x00)			//    ASCII: MSB is 0
		{
			CharLength = 1;						//    1 byte character
			SingleChar[0] = String[i ++];		//    Copy byte to SingleChar[0]
			SingleChar[1] = '\0';				//    Null-terminate SingleChar
		}
		else									//    Chinese: MSB is 1
		{
			CharLength = 2;						//    2 byte character
			SingleChar[0] = String[i ++];		//    Copy first byte to SingleChar[0]
			if (String[i] == '\0') {break;}		//    Check for string end
			SingleChar[1] = String[i ++];		//    Copy second byte to SingleChar[1]
			SingleChar[2] = '\0';				//    Null-terminate SingleChar
		}
#endif

		/*    Display the character*/
		if (CharLength == 1)	//    Single-byte ASCII character
		{
			/*    Use OLED_ShowChar to display it*/
			OLED_ShowChar(X + XOffset, Y, SingleChar[0], FontSize);
			XOffset += FontSize;
		}
		else					//    Multi-byte character (Chinese, etc.)
		{
			/*    Chinese font not available, show placeholder*/
			/*    Font data OLED_CF16x16 is disabled; unable to render multi-byte characters*/
			/* Chinese font (OLED_CF16x16) disabled - show '?' for multi-byte chars */
			OLED_ShowChar(X + XOffset, Y, '?', FontSize);
			XOffset += FontSize;
		}
	}
}

/**
  * Display unsigned decimal number
  *    X: column coordinate (supports -32768~32767 range, visible 0~127)
  *    Y: row coordinate (supports -32768~32767 range, visible 0~63)
  *    Number: value to display (0~4294967295)
  *    Length: number of digits to display (0~10)
  *    FontSize: font selection
  *      Available: OLED_8X16 (8 width x 16 height)
  *                 OLED_6X8  (6 width x 8 height)
  *    Description
  * Note: Must call OLED_Update or OLED_UpdateArea to apply to hardware
  */
void OLED_ShowNum(int16_t X, int16_t Y, uint32_t Number, uint8_t Length, uint8_t FontSize)
{
	uint8_t i;
	for (i = 0; i < Length; i++)		//    Loop through each digit position
	{
		/*    Use OLED_ShowChar to display each digit*/
		/*    Number / OLED_Pow(10, Length - i - 1) % 10 extracts digit at position i*/
		/*    + '0' converts digit to ASCII character*/
		OLED_ShowChar(X + i * FontSize, Y, Number / OLED_Pow(10, Length - i - 1) % 10 + '0', FontSize);
	}
}

/**
  * Display signed decimal number with +/- sign
  *    X: column coordinate (supports -32768~32767 range, visible 0~127)
  *    Y: row coordinate (supports -32768~32767 range, visible 0~63)
  *    Number: value to display (-2147483648~2147483647)
  *    Length: number of digits to display (0~10)
  *    FontSize: font selection
  *      Available: OLED_8X16 (8 width x 16 height)
  *                 OLED_6X8  (6 width x 8 height)
  *    Description
  * Note: Must call OLED_Update or OLED_UpdateArea to apply to hardware
  */
void OLED_ShowSignedNum(int16_t X, int16_t Y, int32_t Number, uint8_t Length, uint8_t FontSize)
{
	uint8_t i;
	uint32_t Number1;

	if (Number >= 0)						//    Number is non-negative
	{
		OLED_ShowChar(X, Y, '+', FontSize);	//    Display + sign
		Number1 = Number;					//    Use positive value
	}
	else									//    Number is negative
	{
		OLED_ShowChar(X, Y, '-', FontSize);	//    Display - sign
		Number1 = -Number;					//    Convert to positive value
	}

	for (i = 0; i < Length; i++)			//    Loop through each digit position
	{
		/*    Use OLED_ShowChar to display each digit*/
		/*    Number1 / OLED_Pow(10, Length - i - 1) % 10 extracts digit at position i*/
		/*    + '0' converts digit to ASCII character*/
		OLED_ShowChar(X + (i + 1) * FontSize, Y, Number1 / OLED_Pow(10, Length - i - 1) % 10 + '0', FontSize);
	}
}

/**
  * Display hexadecimal (base-16) number
  *    X: column coordinate (supports -32768~32767 range, visible 0~127)
  *    Y: row coordinate (supports -32768~32767 range, visible 0~63)
  *    Number: value to display (0x00000000~0xFFFFFFFF)
  *    Length: number of hex digits (0~8)
  *    FontSize: font selection
  *      Available: OLED_8X16 (8 width x 16 height)
  *                 OLED_6X8  (6 width x 8 height)
  *    Description
  * Note: Must call OLED_Update or OLED_UpdateArea to apply to hardware
  */
void OLED_ShowHexNum(int16_t X, int16_t Y, uint32_t Number, uint8_t Length, uint8_t FontSize)
{
	uint8_t i, SingleNumber;
	for (i = 0; i < Length; i++)		//    Loop through each hex digit
	{
		/*    Extract one hex digit (0-15)*/
		SingleNumber = Number / OLED_Pow(16, Length - i - 1) % 16;

		if (SingleNumber < 10)			//    Digit 0-9
		{
			/*    Use OLED_ShowChar to display the digit*/
			/*    + '0' converts to ASCII character*/
			OLED_ShowChar(X + i * FontSize, Y, SingleNumber + '0', FontSize);
		}
		else							//    Digit A-F (10-15)
		{
			/*    Use OLED_ShowChar to display the hex letter*/
			/*    + 'A' converts to ASCII A-F*/
			OLED_ShowChar(X + i * FontSize, Y, SingleNumber - 10 + 'A', FontSize);
		}
	}
}

/**
  * Display binary (base-2) number
  *    X: column coordinate (supports -32768~32767 range, visible 0~127)
  *    Y: row coordinate (supports -32768~32767 range, visible 0~63)
  *    Number: value to display (0x00000000~0xFFFFFFFF)
  *    Length: number of bits to display (0~16)
  *    FontSize: font selection
  *      Available: OLED_8X16 (8 width x 16 height)
  *                 OLED_6X8  (6 width x 8 height)
  *    Description
  * Note: Must call OLED_Update or OLED_UpdateArea to apply to hardware
  */
void OLED_ShowBinNum(int16_t X, int16_t Y, uint32_t Number, uint8_t Length, uint8_t FontSize)
{
	uint8_t i;
	for (i = 0; i < Length; i++)		//    Loop through each bit
	{
		/*    Use OLED_ShowChar to display each bit*/
		/*    Number / OLED_Pow(2, Length - i - 1) % 2 extracts bit at position i*/
		/*    + '0' converts bit to ASCII character*/
		OLED_ShowChar(X + i * FontSize, Y, Number / OLED_Pow(2, Length - i - 1) % 2 + '0', FontSize);
	}
}

/**
  * Display floating-point number
  *    X: column coordinate (supports -32768~32767 range, visible 0~127)
  *    Y: row coordinate (supports -32768~32767 range, visible 0~63)
  *    Number: value to display (-4294967295.0~4294967295.0)
  *    IntLength: number of digits before decimal point (0~10)
  *    FraLength: number of digits after decimal point (0~9); fractional part only
  *    FontSize: font selection
  *      Available: OLED_8X16 (8 width x 16 height)
  *                 OLED_6X8  (6 width x 8 height)
  *    Description
  * Note: Must call OLED_Update or OLED_UpdateArea to apply to hardware
  */
void OLED_ShowFloatNum(int16_t X, int16_t Y, double Number, uint8_t IntLength, uint8_t FraLength, uint8_t FontSize)
{
	uint32_t PowNum, IntNum, FraNum;

	if (Number >= 0)						//    Number is non-negative
	{
		OLED_ShowChar(X, Y, '+', FontSize);	//    Display + sign
	}
	else									//    Number is negative
	{
		OLED_ShowChar(X, Y, '-', FontSize);	//    Display - sign
		Number = -Number;					//    Convert to positive
	}

	/*    Separate integer and fractional parts*/
	IntNum = Number;						//    Extract integer part
	Number -= IntNum;						//    Remove integer to get fractional part only
	PowNum = OLED_Pow(10, FraLength);		//    Calculate scaling factor for precision
	FraNum = round(Number * PowNum);		//    Scale and round fractional part
	IntNum += FraNum / PowNum;				//    Handle rounding overflow to integer part

	/*    Display integer portion*/
	OLED_ShowNum(X + FontSize, Y, IntNum, IntLength, FontSize);

	/*    Display decimal point*/
	OLED_ShowChar(X + (IntLength + 1) * FontSize, Y, '.', FontSize);

	/*    Display fractional portion*/
	OLED_ShowNum(X + (IntLength + 2) * FontSize, Y, FraNum, FraLength, FontSize);
}

/**
  * Display image bitmap at position
  *    X: column coordinate (supports -32768~32767 range, visible 0~127)
  *    Y: row coordinate (supports -32768~32767 range, visible 0~63)
  *    Width: image width in pixels (0~128)
  *    Height: image height in pixels (0~64)
  *    Image: pointer to image bitmap data
  *    Description
  * Note: Must call OLED_Update or OLED_UpdateArea to apply to hardware
  */
void OLED_ShowImage(int16_t X, int16_t Y, uint8_t Width, uint8_t Height, const uint8_t *Image)
{
	uint8_t i = 0, j = 0;
	int16_t Page, Shift;

	/*    Clear the area first*/
	OLED_ClearArea(X, Y, Width, Height);

	/*    Calculate pages (8-pixel rows) spanned by image*/
	/*(Height - 1) / 8 + 1 equals Height / 8 with rounding up*/
	for (j = 0; j < (Height - 1) / 8 + 1; j ++)
	{
		/*    Process each column of the image*/
		for (i = 0; i < Width; i ++)
		{
			if (X + i >= 0 && X + i <= 127)		//    Boundary check
			{
				/*    Calculate page offset and bit shift within page*/
				Page = Y / 8;
				Shift = Y % 8;
				if (Y < 0)
				{
					Page -= 1;
					Shift += 8;
				}

				if (Page + j >= 0 && Page + j <= 7)		//    Boundary check
				{
					/*    Draw image bits to upper part of byte*/
					OLED_DisplayBuf[Page + j][X + i] |= Image[j * Width + i] << (Shift);
				}

				if (Page + j + 1 >= 0 && Page + j + 1 <= 7)		//    Boundary check
				{
					/*    Draw image bits to lower part of next byte*/
					OLED_DisplayBuf[Page + j + 1][X + i] |= Image[j * Width + i] >> (8 - Shift);
				}
			}
		}
	}
}

/**
  * Display formatted text using printf-style format string
  *    X: column coordinate (supports -32768~32767 range, visible 0~127)
  *    Y: row coordinate (supports -32768~32767 range, visible 0~63)
  *    FontSize: font selection
  *      Available: OLED_8X16 (8 width x 16 height)
  *                 OLED_6X8  (6 width x 8 height)
  *    format: format string (same as printf)
  *    ...: variable arguments for format string
  *    Description
  * Note: Chinese font (OLED_CF16x16) requires external file OLED_Data.c
  *      Full multi-byte UTF8/GB2312 support requires font data not included
  *      Without font data, multi-byte characters display as '?'
  *         With OLED_8X16, multi-byte chars display as 16x16 blocks
  *         With OLED_6X8, multi-byte chars display as 6x8 blocks
  * Note: Must call OLED_Update or OLED_UpdateArea to apply to hardware
  */
void OLED_Printf(int16_t X, int16_t Y, uint8_t FontSize, char *format, ...)
{
	char String[256];						//    Temporary string buffer
	va_list arg;							//    Variable argument list
	va_start(arg, format);					//    Initialize arg list starting from format
	vsprintf(String, format, arg);			//    Format string and store in String buffer
	va_end(arg);							//    Clean up arg list
	OLED_ShowString(X, Y, String, FontSize);//    Display the formatted string
}

/**
  * Draw a pixel (point) at position
  *    X: column coordinate (supports -32768~32767 range, visible 0~127)
  *    Y: row coordinate (supports -32768~32767 range, visible 0~63)
  *    Description
  * Note: Must call OLED_Update or OLED_UpdateArea to apply to hardware
  */
void OLED_DrawPoint(int16_t X, int16_t Y)
{
	if (X >= 0 && X <= 127 && Y >=0 && Y <= 63)		//    Boundary check
	{
		/*    Set the pixel bit in the display buffer to 1 (on)*/
		OLED_DisplayBuf[Y / 8][X] |= 0x01 << (Y % 8);
	}
}

/**
  * Read pixel state at position
  *    X: column coordinate (supports -32768~32767 range, visible 0~127)
  *    Y: row coordinate (supports -32768~32767 range, visible 0~63)
  *    Returns: 1 if pixel is on, 0 if off
  */
uint8_t OLED_GetPoint(int16_t X, int16_t Y)
{
	if (X >= 0 && X <= 127 && Y >=0 && Y <= 63)		//    Boundary check
	{
		/*    Read the pixel bit from display buffer*/
		if (OLED_DisplayBuf[Y / 8][X] & 0x01 << (Y % 8))
		{
			return 1;	//    Pixel is on
		}
	}

	return 0;		//    Pixel is off or out of bounds
}

/**
  * Draw line between two points
  *    X0: start column (supports -32768~32767 range, visible 0~127)
  *    Y0: start row (supports -32768~32767 range, visible 0~63)
  *    X1: end column (supports -32768~32767 range, visible 0~127)
  *    Y1: end row (supports -32768~32767 range, visible 0~63)
  *    Description
  * Note: Must call OLED_Update or OLED_UpdateArea to apply to hardware
  */
void OLED_DrawLine(int16_t X0, int16_t Y0, int16_t X1, int16_t Y1)
{
	int16_t x, y, dx, dy, d, incrE, incrNE, temp;
	int16_t x0 = X0, y0 = Y0, x1 = X1, y1 = Y1;
	uint8_t yflag = 0, xyflag = 0;

	if (y0 == y1)		//    Horizontal line
	{
		/*    Ensure x0 < x1 for left-to-right drawing*/
		if (x0 > x1) {temp = x0; x0 = x1; x1 = temp;}

		/*    Draw pixels along X axis*/
		for (x = x0; x <= x1; x ++)
		{
			OLED_DrawPoint(x, y0);	//    Set pixel
		}
	}
	else if (x0 == x1)	//    Vertical line
	{
		/*    Ensure y0 < y1 for top-to-bottom drawing*/
		if (y0 > y1) {temp = y0; y0 = y1; y1 = temp;}

		/*    Draw pixels along Y axis*/
		for (y = y0; y <= y1; y ++)
		{
			OLED_DrawPoint(x0, y);	//    Set pixel
		}
	}
	else				//    Diagonal line
	{
		/*    Use Bresenham algorithm for efficient line rasterization
		 Reference: https://www.cs.montana.edu/courses/spring2009/425/dslectures/Bresenham.pdf
		 Tutorial: https://www.bilibili.com/video/BV1364y1d7Lo*/

		if (x0 > x1)	//    Ensure x0 < x1
		{
			/*    Swap endpoints for consistent processing*/
			/*    Ensures line is drawn left-to-right with proper coordinate mapping*/
			temp = x0; x0 = x1; x1 = temp;
			temp = y0; y0 = y1; y1 = temp;
		}

		if (y0 > y1)	//    Ensure y0 < y1
		{
			/*    Flip Y coordinates to handle downward lines*/
			/*    Adjust signs consistently for upper-left to lower-right direction*/
			y0 = -y0;
			y1 = -y1;

			/*    Set flag to track Y-axis flip when drawing final pixels*/
			yflag = 1;
		}

		if (y1 - y0 > x1 - x0)	//    Line is more vertical than horizontal
		{
			/*    Swap X and Y to normalize to gentle slope (0-45 degrees)*/
			/*    Reduces calculation complexity by working with shallow angles*/
			temp = x0; x0 = y0; y0 = temp;
			temp = x1; x1 = y1; y1 = temp;

			/*    Set flag to track X-Y swap when drawing final pixels*/
			xyflag = 1;
		}

		/*    Bresenham algorithm main loop*/
		/*    Works with normalized line in 0-45 degree range*/
		dx = x1 - x0;
		dy = y1 - y0;
		incrE = 2 * dy;
		incrNE = 2 * (dy - dx);
		d = 2 * dy - dx;
		x = x0;
		y = y0;

		/*    Draw initial point with appropriate coordinate transforms*/
		if (yflag && xyflag){OLED_DrawPoint(y, -x);}
		else if (yflag)		{OLED_DrawPoint(x, -y);}
		else if (xyflag)	{OLED_DrawPoint(y, x);}
		else				{OLED_DrawPoint(x, y);}

		while (x < x1)		//    Loop until end of line
		{
			x ++;
			if (d < 0)		//    Decision: move East only
			{
				d += incrE;
			}
			else			//    Decision: move Northeast (East + North)
			{
				y ++;
				d += incrNE;
			}

			/*    Draw point with appropriate coordinate transforms*/
			if (yflag && xyflag){OLED_DrawPoint(y, -x);}
			else if (yflag)		{OLED_DrawPoint(x, -y);}
			else if (xyflag)	{OLED_DrawPoint(y, x);}
			else				{OLED_DrawPoint(x, y);}
		}
	}
}

/**
  * Draw rectangle outline or filled
  *    X: column coordinate (supports -32768~32767 range, visible 0~127)
  *    Y: row coordinate (supports -32768~32767 range, visible 0~63)
  *    Width: rectangle width in pixels (0~128)
  *    Height: rectangle height in pixels (0~64)
  *    IsFilled: fill mode selection
  *      Available: OLED_UNFILLED (outline only)
  *                 OLED_FILLED   (solid fill)
  *    Description
  * Note: Must call OLED_Update or OLED_UpdateArea to apply to hardware
  */
void OLED_DrawRectangle(int16_t X, int16_t Y, uint8_t Width, uint8_t Height, uint8_t IsFilled)
{
	int16_t i, j;
	if (!IsFilled)		//    Draw outline only
	{
		/*    Draw top and bottom horizontal edges*/
		for (i = X; i < X + Width; i ++)
		{
			OLED_DrawPoint(i, Y);
			OLED_DrawPoint(i, Y + Height - 1);
		}
		/*    Draw left and right vertical edges*/
		for (i = Y; i < Y + Height; i ++)
		{
			OLED_DrawPoint(X, i);
			OLED_DrawPoint(X + Width - 1, i);
		}
	}
	else				//    Draw filled rectangle
	{
		/*    Scan X axis*/
		for (i = X; i < X + Width; i ++)
		{
			/*    Scan Y axis*/
			for (j = Y; j < Y + Height; j ++)
			{
				/*    Set all pixels in rectangle*/
				OLED_DrawPoint(i, j);
			}
		}
	}
}

/**
  * Draw triangle outline or filled
  *    X0: first vertex column (supports -32768~32767 range, visible 0~127)
  *    Y0: first vertex row (supports -32768~32767 range, visible 0~63)
  *    X1: second vertex column (supports -32768~32767 range, visible 0~127)
  *    Y1: second vertex row (supports -32768~32767 range, visible 0~63)
  *    X2: third vertex column (supports -32768~32767 range, visible 0~127)
  *    Y2: third vertex row (supports -32768~32767 range, visible 0~63)
  *    IsFilled: fill mode selection
  *      Available: OLED_UNFILLED (outline only)
  *                 OLED_FILLED   (solid fill)
  *    Description
  * Note: Must call OLED_Update or OLED_UpdateArea to apply to hardware
  */
void OLED_DrawTriangle(int16_t X0, int16_t Y0, int16_t X1, int16_t Y1, int16_t X2, int16_t Y2, uint8_t IsFilled)
{
	int16_t minx = X0, miny = Y0, maxx = X0, maxy = Y0;
	int16_t i, j;
	int16_t vx[] = {X0, X1, X2};
	int16_t vy[] = {Y0, Y1, Y2};

	if (!IsFilled)			//    Draw outline only
	{
		/*    Draw three edges connecting the vertices*/
		OLED_DrawLine(X0, Y0, X1, Y1);
		OLED_DrawLine(X0, Y0, X2, Y2);
		OLED_DrawLine(X1, Y1, X2, Y2);
	}
	else					//    Draw filled triangle
	{
		/*    Find minimum bounding box X and Y*/
		if (X1 < minx) {minx = X1;}
		if (X2 < minx) {minx = X2;}
		if (Y1 < miny) {miny = Y1;}
		if (Y2 < miny) {miny = Y2;}

		/*    Find maximum bounding box X and Y*/
		if (X1 > maxx) {maxx = X1;}
		if (X2 > maxx) {maxx = X2;}
		if (Y1 > maxy) {maxy = Y1;}
		if (Y2 > maxy) {maxy = Y2;}

		/*    Test all pixels in bounding box using point-in-polygon test*/
		/*    Fill triangle by setting pixels inside the polygon*/
		/*    Scan X axis*/
		for (i = minx; i <= maxx; i ++)
		{
			/*    Scan Y axis*/
			for (j = miny; j <= maxy; j ++)
			{
				/*    Use OLED_pnpoly to test if point is inside triangle*/
				/*    Set pixel only if point is inside polygon*/
				if (OLED_pnpoly(3, vx, vy, i, j)) {OLED_DrawPoint(i, j);}
			}
		}
	}
}

/**
  * Draw circle outline or filled
  *    X: center column (supports -32768~32767 range, visible 0~127)
  *    Y: center row (supports -32768~32767 range, visible 0~63)
  *    Radius: circle radius in pixels (0~255)
  *    IsFilled: fill mode selection
  *      Available: OLED_UNFILLED (outline only)
  *                 OLED_FILLED   (solid fill)
  *    Description
  * Note: Must call OLED_Update or OLED_UpdateArea to apply to hardware
  */
void OLED_DrawCircle(int16_t X, int16_t Y, uint8_t Radius, uint8_t IsFilled)
{
	int16_t x, y, d, j;

	/*    Use Bresenham algorithm for efficient circle rasterization
	 Reference: https://www.cs.montana.edu/courses/spring2009/425/dslectures/Bresenham.pdf
	 Tutorial: https://www.bilibili.com/video/BV1VM4y1u7wJ*/

	d = 1 - Radius;
	x = 0;
	y = Radius;

	/*    Draw initial points in octant symmetry*/
	OLED_DrawPoint(X + x, Y + y);
	OLED_DrawPoint(X - x, Y - y);
	OLED_DrawPoint(X + y, Y + x);
	OLED_DrawPoint(X - y, Y - x);

	if (IsFilled)		//    Draw filled circle
	{
		/*    Draw vertical lines from top to bottom*/
		for (j = -y; j < y; j ++)
		{
			/*    Set pixels along vertical diameter*/
			OLED_DrawPoint(X, Y + j);
		}
	}

	while (x < y)		//    Loop while x < y (until octant boundary)
	{
		x ++;
		if (d < 0)		//    Decision: move East only
		{
			d += 2 * x + 1;
		}
		else			//    Decision: move Northeast (East + North)
		{
			y --;
			d += 2 * (x - y) + 1;
		}

		/*    Draw points in 8-way symmetry*/
		OLED_DrawPoint(X + x, Y + y);
		OLED_DrawPoint(X + y, Y + x);
		OLED_DrawPoint(X - x, Y - y);
		OLED_DrawPoint(X - y, Y - x);
		OLED_DrawPoint(X + x, Y - y);
		OLED_DrawPoint(X + y, Y - x);
		OLED_DrawPoint(X - x, Y + y);
		OLED_DrawPoint(X - y, Y + x);

		if (IsFilled)	//    Draw filled circle
		{
			/*    Draw vertical scan lines on right side*/
			for (j = -y; j < y; j ++)
			{
				/*    Set pixels along vertical line*/
				OLED_DrawPoint(X + x, Y + j);
				OLED_DrawPoint(X - x, Y + j);
			}

			/*    Draw vertical scan lines on top/bottom*/
			for (j = -x; j < x; j ++)
			{
				/*    Set pixels along vertical line*/
				OLED_DrawPoint(X - y, Y + j);
				OLED_DrawPoint(X + y, Y + j);
			}
		}
	}
}

/**
  * Draw ellipse outline or filled
  *    X: center column (supports -32768~32767 range, visible 0~127)
  *    Y: center row (supports -32768~32767 range, visible 0~63)
  *    A: semi-major axis in X direction (0~255)
  *    B: semi-minor axis in Y direction (0~255)
  *    IsFilled: fill mode selection
  *      Available: OLED_UNFILLED (outline only)
  *                 OLED_FILLED   (solid fill)
  *    Description
  * Note: Must call OLED_Update or OLED_UpdateArea to apply to hardware
  */
void OLED_DrawEllipse(int16_t X, int16_t Y, uint8_t A, uint8_t B, uint8_t IsFilled)
{
	int16_t x, y, j;
	int16_t a = A, b = B;
	float d1, d2;

	/*    Use Bresenham algorithm for ellipse rasterization
	 Reference: https://blog.csdn.net/myf_666/article/details/128167392*/

	x = 0;
	y = b;
	d1 = b * b + a * a * (-b + 0.5);

	if (IsFilled)	//    Draw filled ellipse
	{
		/*    Draw vertical lines on left/right sides*/
		for (j = -y; j < y; j ++)
		{
			/*    Set pixels along vertical line*/
			OLED_DrawPoint(X, Y + j);
			OLED_DrawPoint(X, Y + j);
		}
	}

	/*    Draw initial points in 4-way symmetry*/
	OLED_DrawPoint(X + x, Y + y);
	OLED_DrawPoint(X - x, Y - y);
	OLED_DrawPoint(X - x, Y + y);
	OLED_DrawPoint(X + x, Y - y);

	/*    Main loop for upper region*/
	while (b * b * (x + 1) < a * a * (y - 0.5))
	{
		if (d1 <= 0)		//    Decision: move East only
		{
			d1 += b * b * (2 * x + 3);
		}
		else				//    Decision: move Northeast (East + North)
		{
			d1 += b * b * (2 * x + 3) + a * a * (-2 * y + 2);
			y --;
		}
		x ++;

		if (IsFilled)	//    Draw filled ellipse
		{
			/*    Draw vertical scan lines on right side*/
			for (j = -y; j < y; j ++)
			{
				/*    Set pixels along vertical line*/
				OLED_DrawPoint(X + x, Y + j);
				OLED_DrawPoint(X - x, Y + j);
			}
		}

		/*    Draw points in 4-way symmetry on boundary*/
		OLED_DrawPoint(X + x, Y + y);
		OLED_DrawPoint(X - x, Y - y);
		OLED_DrawPoint(X - x, Y + y);
		OLED_DrawPoint(X + x, Y - y);
	}

	/*    Main loop for lower region*/
	d2 = b * b * (x + 0.5) * (x + 0.5) + a * a * (y - 1) * (y - 1) - a * a * b * b;

	while (y > 0)
	{
		if (d2 <= 0)		//    Decision: move East only
		{
			d2 += b * b * (2 * x + 2) + a * a * (-2 * y + 3);
			x ++;

		}
		else				//    Decision: move South only
		{
			d2 += a * a * (-2 * y + 3);
		}
		y --;

		if (IsFilled)	//    Draw filled ellipse
		{
			/*    Draw vertical scan lines on right/left*/
			for (j = -y; j < y; j ++)
			{
				/*    Set pixels along vertical line*/
				OLED_DrawPoint(X + x, Y + j);
				OLED_DrawPoint(X - x, Y + j);
			}
		}

		/*    Draw points in 4-way symmetry on boundary*/
		OLED_DrawPoint(X + x, Y + y);
		OLED_DrawPoint(X - x, Y - y);
		OLED_DrawPoint(X - x, Y + y);
		OLED_DrawPoint(X + x, Y - y);
	}
}

/**
  * Draw circular arc outline or filled
  *    X: center column (supports -32768~32767 range, visible 0~127)
  *    Y: center row (supports -32768~32767 range, visible 0~63)
  *    Radius: arc radius in pixels (0~255)
  *    StartAngle: arc starting angle in degrees (-180~180)
  *    Angle 0 is rightward, 180/-180 is leftward; proper range wrapping applied
  *    EndAngle: arc ending angle in degrees (-180~180)
  *    Angle 0 is rightward, 180/-180 is leftward; proper range wrapping applied
  *    IsFilled: fill mode selection
  *      Available: OLED_UNFILLED (outline only)
  *                 OLED_FILLED   (solid fill)
  *    Description
  * Note: Must call OLED_Update or OLED_UpdateArea to apply to hardware
  */
void OLED_DrawArc(int16_t X, int16_t Y, uint8_t Radius, int16_t StartAngle, int16_t EndAngle, uint8_t IsFilled)
{
	int16_t x, y, d, j;

	/*    Use Bresenham algorithm for arc rasterization*/

	d = 1 - Radius;
	x = 0;
	y = Radius;

	/*    Check each octant point to see if in angle range; draw if yes*/
	if (OLED_IsInAngle(x, y, StartAngle, EndAngle))	{OLED_DrawPoint(X + x, Y + y);}
	if (OLED_IsInAngle(-x, -y, StartAngle, EndAngle)) {OLED_DrawPoint(X - x, Y - y);}
	if (OLED_IsInAngle(y, x, StartAngle, EndAngle)) {OLED_DrawPoint(X + y, Y + x);}
	if (OLED_IsInAngle(-y, -x, StartAngle, EndAngle)) {OLED_DrawPoint(X - y, Y - x);}

	if (IsFilled)	//    Draw filled arc
	{
		/*    Draw vertical lines from top to bottom*/
		for (j = -y; j < y; j ++)
		{
			/*    Check if point is within angle range; draw if yes*/
			if (OLED_IsInAngle(0, j, StartAngle, EndAngle)) {OLED_DrawPoint(X, Y + j);}
		}
	}

	while (x < y)		//    Loop while x < y (until octant boundary)
	{
		x ++;
		if (d < 0)		//    Decision: move East only
		{
			d += 2 * x + 1;
		}
		else			//    Decision: move Northeast (East + North)
		{
			y --;
			d += 2 * (x - y) + 1;
		}

		/*    Check each octant point to see if in angle range; draw if yes*/
		if (OLED_IsInAngle(x, y, StartAngle, EndAngle)) {OLED_DrawPoint(X + x, Y + y);}
		if (OLED_IsInAngle(y, x, StartAngle, EndAngle)) {OLED_DrawPoint(X + y, Y + x);}
		if (OLED_IsInAngle(-x, -y, StartAngle, EndAngle)) {OLED_DrawPoint(X - x, Y - y);}
		if (OLED_IsInAngle(-y, -x, StartAngle, EndAngle)) {OLED_DrawPoint(X - y, Y - x);}
		if (OLED_IsInAngle(x, -y, StartAngle, EndAngle)) {OLED_DrawPoint(X + x, Y - y);}
		if (OLED_IsInAngle(y, -x, StartAngle, EndAngle)) {OLED_DrawPoint(X + y, Y - x);}
		if (OLED_IsInAngle(-x, y, StartAngle, EndAngle)) {OLED_DrawPoint(X - x, Y + y);}
		if (OLED_IsInAngle(-y, x, StartAngle, EndAngle)) {OLED_DrawPoint(X - y, Y + x);}

		if (IsFilled)	//    Draw filled arc
		{
			/*    Draw vertical scan lines on right side*/
			for (j = -y; j < y; j ++)
			{
				/*    Check if point is within angle range; draw if yes*/
				if (OLED_IsInAngle(x, j, StartAngle, EndAngle)) {OLED_DrawPoint(X + x, Y + j);}
				if (OLED_IsInAngle(-x, j, StartAngle, EndAngle)) {OLED_DrawPoint(X - x, Y + j);}
			}

			/*    Draw vertical scan lines on top/bottom*/
			for (j = -x; j < x; j ++)
			{
				/*    Check if point is within angle range; draw if yes*/
				if (OLED_IsInAngle(-y, j, StartAngle, EndAngle)) {OLED_DrawPoint(X - y, Y + j);}
				if (OLED_IsInAngle(y, j, StartAngle, EndAngle)) {OLED_DrawPoint(X + y, Y + j);}
			}
		}
	}
}

/**Drawing Functions **/


/*****************************End of File | Thank You | Goodbye*****/
/**************************jiangxiekeji.com**************************/
