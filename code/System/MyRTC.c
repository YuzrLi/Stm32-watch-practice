#include "stm32f10x.h"                  // Device header
#include <time.h>

int MyRTC_Time[] = {2026, 4, 23, 11, 56, 55};	// Global time array: year, month, day, hour, minute, second

void MyRTC_SetTime(void);				// Function declaration

/**
  * Function : RTC initialization
  * Arguments: none
  * Returns  : none
  */
void MyRTC_Init(void)
{
	/* Enable clocks */
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);		// Enable PWR clock
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_BKP, ENABLE);		// Enable BKP clock

	/* Enable access to backup registers */
	PWR_BackupAccessCmd(ENABLE);							// Use PWR to enable backup register access

	if (BKP_ReadBackupRegister(BKP_DR1) != 0xA5A5)			// Check backup register flag to determine if this is the first RTC configuration
																// If true, perform first-time RTC setup
	{
		RCC_LSEConfig(RCC_LSE_ON);							// Enable LSE clock
		while (RCC_GetFlagStatus(RCC_FLAG_LSERDY) != SET);	// Wait for LSE to be ready

		RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE);				// Select LSE as RTC clock source
		RCC_RTCCLKCmd(ENABLE);								// Enable RTC clock

		RTC_WaitForSynchro();								// Wait for synchronization
		RTC_WaitForLastTask();								// Wait for last operation to complete

		RTC_SetPrescaler(32768 - 1);						// Set RTC prescaler so the count frequency is 1Hz
		RTC_WaitForLastTask();								// Wait for last operation to complete

		MyRTC_SetTime();									// Set time: writes the global array values into the RTC hardware

		BKP_WriteBackupRegister(BKP_DR1, 0xA5A5);			// Write flag to backup register to indicate RTC has been configured
	}
	else													// RTC already configured — resume from stored time
	{
		RTC_WaitForSynchro();								// Wait for synchronization
		RTC_WaitForLastTask();								// Wait for last operation to complete
	}
}

// If LSE fails to start and the program hangs in the init function,
// replace the init function with the code below to use LSI as RTCCLK.
// Note: LSI cannot be powered by the backup supply, so RTC will stop when main power is cut.
/*
void MyRTC_Init(void)
{
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_BKP, ENABLE);

	PWR_BackupAccessCmd(ENABLE);

	if (BKP_ReadBackupRegister(BKP_DR1) != 0xA5A5)
	{
		RCC_LSICmd(ENABLE);
		while (RCC_GetFlagStatus(RCC_FLAG_LSIRDY) != SET);

		RCC_RTCCLKConfig(RCC_RTCCLKSource_LSI);
		RCC_RTCCLKCmd(ENABLE);

		RTC_WaitForSynchro();
		RTC_WaitForLastTask();

		RTC_SetPrescaler(40000 - 1);
		RTC_WaitForLastTask();

		MyRTC_SetTime();

		BKP_WriteBackupRegister(BKP_DR1, 0xA5A5);
	}
	else
	{
		RCC_LSICmd(ENABLE);				// LSI must be re-enabled on every boot even if not first config
		while (RCC_GetFlagStatus(RCC_FLAG_LSIRDY) != SET);

		RCC_RTCCLKConfig(RCC_RTCCLKSource_LSI);
		RCC_RTCCLKCmd(ENABLE);

		RTC_WaitForSynchro();
		RTC_WaitForLastTask();
	}
}*/

/**
  * Function : Set RTC time
  * Arguments: none
  * Returns  : none
  * Note     : After calling this function, the values in the global array are written to the RTC hardware
  */
void MyRTC_SetTime(void)
{
	time_t time_cnt;		// Unix timestamp (seconds since epoch)
	struct tm time_date;	// Calendar time structure

	time_date.tm_year = MyRTC_Time[0] - 1900;		// Populate struct from global array
	time_date.tm_mon = MyRTC_Time[1] - 1;
	time_date.tm_mday = MyRTC_Time[2];
	time_date.tm_hour = MyRTC_Time[3];
	time_date.tm_min = MyRTC_Time[4];
	time_date.tm_sec = MyRTC_Time[5];

	time_cnt = mktime(&time_date) - 8 * 60 * 60;	// Convert to Unix timestamp; subtract 8h for UTC+8 timezone

	RTC_SetCounter(time_cnt);						// Write timestamp into RTC counter (CNT register)
	RTC_WaitForLastTask();							// Wait for last operation to complete
}

/**
  * Function : Read RTC time
  * Arguments: none
  * Returns  : none
  * Note     : After calling this function, the RTC hardware time is refreshed into the global array
  */
void MyRTC_ReadTime(void)
{
	time_t time_cnt;		// Unix timestamp
	struct tm time_date;	// Calendar time structure

	time_cnt = RTC_GetCounter() + 8 * 60 * 60;		// Read RTC counter and add 8h offset for UTC+8

	time_date = *localtime(&time_cnt);				// Convert timestamp to calendar format

	MyRTC_Time[0] = time_date.tm_year + 1900;		// Write back to global array
	MyRTC_Time[1] = time_date.tm_mon + 1;
	MyRTC_Time[2] = time_date.tm_mday;
	MyRTC_Time[3] = time_date.tm_hour;
	MyRTC_Time[4] = time_date.tm_min;
	MyRTC_Time[5] = time_date.tm_sec;
}
