#include "stm32f10x.h"                  // Device header

/**
  * Function : Timer interrupt initialization
  * Arguments: none
  * Returns  : none
  */
void Timer_Init(void)
{
	/* Enable clock */
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);			// Enable TIM2 clock

	/* Configure clock source */
	TIM_InternalClockConfig(TIM2);		// Use internal clock for TIM2 (this is also the default if not called)

	/* Time base unit initialization */
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;				// Declare struct
	TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;		// No clock division (used for filter clock, does not affect time base)
	TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;	// Count up
	TIM_TimeBaseInitStructure.TIM_Period = 100 - 1;					// Auto-reload value (ARR)
	TIM_TimeBaseInitStructure.TIM_Prescaler = 720 - 1;				// Prescaler value (PSC): 72MHz / 720 = 100kHz
	TIM_TimeBaseInitStructure.TIM_RepetitionCounter = 0;			// Repetition counter (advanced timers only)
	TIM_TimeBaseInit(TIM2, &TIM_TimeBaseInitStructure);				// Apply configuration to TIM2

	/* Interrupt output configuration */
	TIM_ClearFlag(TIM2, TIM_FLAG_Update);						// Clear update flag
																// TIM_TimeBaseInit generates an update event at the end,
																// so we clear the flag here to avoid an immediate interrupt on enable.
																// This step can be skipped if the first spurious interrupt is acceptable.

	TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);					// Enable TIM2 update interrupt

	/* NVIC interrupt group configuration */
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);				// Set NVIC priority group 2
																// Preemption priority: 0~3, sub-priority: 0~3
																// This only needs to be called once for the whole project.
																// If called multiple times, the last call wins.

	/* NVIC configuration */
	NVIC_InitTypeDef NVIC_InitStructure;						// Declare struct
	NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;				// Select TIM2 interrupt line
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;				// Enable interrupt line
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;	// Preemption priority: 2
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;			// Sub-priority: 1
	NVIC_Init(&NVIC_InitStructure);								// Apply NVIC configuration

	/* Enable timer */
	TIM_Cmd(TIM2, ENABLE);			// Enable TIM2 — timer starts running
}

/* Timer interrupt handler template (copy to wherever it is used):
void TIM2_IRQHandler(void)
{
	if (TIM_GetITStatus(TIM2, TIM_IT_Update) == SET)
	{

		TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
	}
}
*/
