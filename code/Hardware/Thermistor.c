#include "stm32f10x.h"                  // Device header
#include "AD.h"
#include <math.h>

/**
  * Function : Thermistor / temperature sensor initialization
  * Arguments: none
  * Returns  : none
  * Note     : Calls AD_Init() to configure PA0 as analog input (ADC1 CH0).
  *            Connect the sensor's AO pin to PA0.
  *            Circuit: VCC --[R1=10K]-- AO(PA0) --[NTC]-- GND
  */
void Thermistor_Init(void)
{
	AD_Init();		// Configure PA0 and ADC1 for single-conversion mode
}

/**
  * Function : Read ambient temperature from NTC thermistor
  * Arguments: none
  * Returns  : Temperature in degrees Celsius (float)
  * Note     : Uses the Beta (B-parameter) equation:
  *              1/T = 1/T0 + (1/B) * ln(R_NTC / R0)
  *            Parameters for the YL-38 module NTC thermistor:
  *              R0 = 10000 ohm  (resistance at 25 degrees C)
  *              T0 = 298.15 K   (25 degrees C in Kelvin)
  *              B  = 3435 K     (Beta coefficient)
  *              R1 = 10000 ohm  (voltage divider pull-up resistor on the module)
  */
float Get_Temperature(void)
{
	uint16_t adcVal = AD_GetValue();

	/* Guard against divide-by-zero (ADC at full scale = short circuit) */
	if (adcVal >= 4095) return -99.0f;
	if (adcVal == 0)    return  99.0f;

	/* Calculate NTC resistance from voltage divider:
	   V_AO = VCC * R_NTC / (R1 + R_NTC)
	   ADC  = 4095 * R_NTC / (R1 + R_NTC)
	   R_NTC = R1 * ADC / (4095 - ADC)                 */
	float R_NTC = 10000.0f * (float)adcVal / (4095.0f - (float)adcVal);

	/* Beta equation: T(K) = 1 / ( 1/T0 + ln(R/R0)/B ) */
	float T0 = 298.15f;		// Reference temperature: 25 degrees C in Kelvin
	float R0 = 10000.0f;	// NTC resistance at 25 degrees C
	float B  = 3435.0f;		// Beta coefficient

	float T_kelvin = 1.0f / (1.0f / T0 + logf(R_NTC / R0) / B);

	return T_kelvin - 273.15f;	// Convert Kelvin to Celsius
}
