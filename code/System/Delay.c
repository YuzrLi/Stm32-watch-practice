#include "stm32f10x.h"

/**
  * @brief  Microsecond delay
  * @param  xus  Delay duration in microseconds, range: 0~233015
  * @retval none
  */
void Delay_us(uint32_t xus)
{
	SysTick->LOAD = 72 * xus;				// Set reload value
	SysTick->VAL = 0x00;					// Clear current counter value
	SysTick->CTRL = 0x00000005;				// Select HCLK as clock source and start timer
	while(!(SysTick->CTRL & 0x00010000));	// Wait until count reaches zero
	SysTick->CTRL = 0x00000004;				// Stop timer
}

/**
  * @brief  Millisecond delay
  * @param  xms  Delay duration in milliseconds, range: 0~4294967295
  * @retval none
  */
void Delay_ms(uint32_t xms)
{
	while(xms--)
	{
		Delay_us(1000);
	}
}

/**
  * @brief  Second delay
  * @param  xs  Delay duration in seconds, range: 0~4294967295
  * @retval none
  */
void Delay_s(uint32_t xs)
{
	while(xs--)
	{
		Delay_ms(1000);
	}
}
