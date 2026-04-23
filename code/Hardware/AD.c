#include "stm32f10x.h"                  // Device header

/**
  * Function : ADC initialization
  * Arguments: none
  * Returns  : none
  */
void AD_Init(void)
{
	/* Enable clocks */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);	// Enable ADC1 clock
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);	// Enable GPIOA clock

	/* Configure ADC clock */
	RCC_ADCCLKConfig(RCC_PCLK2_Div6);						// ADCCLK = 72MHz / 6 = 12MHz

	/* GPIO initialization */
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);					// Configure PA0 as analog input

	/* Regular group channel configuration */
	ADC_RegularChannelConfig(ADC1, ADC_Channel_0, 1, ADC_SampleTime_55Cycles5);	// Rank 1 = Channel 0

	/* ADC initialization */
	ADC_InitTypeDef ADC_InitStructure;						// Declare struct
	ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;		// Independent mode (ADC1 only)
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;	// Right-align data
	ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;	// Software trigger (no external trigger)
	ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;		// Single conversion mode (stop after one sequence)
	ADC_InitStructure.ADC_ScanConvMode = DISABLE;			// Scan mode disabled (only convert rank 1)
	ADC_InitStructure.ADC_NbrOfChannel = 1;					// Number of channels: 1 (only needed > 1 in scan mode)
	ADC_Init(ADC1, &ADC_InitStructure);						// Apply configuration to ADC1

	/* Enable ADC */
	ADC_Cmd(ADC1, ENABLE);									// Enable ADC1

	/* ADC calibration */
	ADC_ResetCalibration(ADC1);								// Reset calibration registers (hardware performs calibration automatically)
	while (ADC_GetResetCalibrationStatus(ADC1) == SET);
	ADC_StartCalibration(ADC1);
	while (ADC_GetCalibrationStatus(ADC1) == SET);
}

/**
  * Function : Get ADC conversion result
  * Arguments: none
  * Returns  : ADC result, range: 0~4095
  */
uint16_t AD_GetValue(void)
{
	ADC_SoftwareStartConvCmd(ADC1, ENABLE);					// Trigger a single software conversion
	while (ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) == RESET);	// Wait for End-Of-Conversion flag
	return ADC_GetConversionValue(ADC1);					// Read and return conversion result
}
