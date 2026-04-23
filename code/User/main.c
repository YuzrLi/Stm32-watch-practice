#include "stm32f10x.h"                  // Device header
#include "Delay.h"
#include "OLED.h"
#include "menu.h"
#include "Timer.h"
#include "Key.h"
#include "dino.h"
/**
  * OLED coordinate system:
  * Origin (0, 0) is at the top-left corner.
  * X axis runs horizontally, range: 0~127
  * Y axis runs vertically,   range: 0~63
  *
  *       0             X axis          127
  *      .------------------------------->
  *    0 |
  *      |
  *      |
  *      |
  * Y axis
  *      |
  *      |
  *      |
  *   63 |
  *      v
  *
  */

int main(void)
{
	/* OLED initialization */
	OLED_Init();
	OLED_Clear();
	Peripheral_Init();	// Initialize all peripherals
	int clkflag1;

	extern int press_time;
	extern uint8_t Key_Num;
	Timer_Init();
	uint8_t begin_flag=1;
	while (1)
	{
//		OLED_ShowNum(64,0,press_time,4,OLED_6X8);
//		OLED_ShowNum(64,8,Key_Num,1,OLED_6X8);
//		OLED_Update();

		clkflag1=First_Page_Clock();
		if(begin_flag==1)
		{
			clkflag1=0;
			begin_flag=0;
		}
		if(clkflag1==1){Menu();}		// Open main menu
		else if(clkflag1==2){SettingPage();}	// Open settings page

	}
}

/* TIM2 interrupt handler — copy this template to wherever it is used */
void TIM2_IRQHandler(void)
{
	if (TIM_GetITStatus(TIM2, TIM_IT_Update) == SET)
	{
		Key3_Tick();
		Key_Tick();
		StopWatch_Tick();
		Dino_Tick();
		TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
	}
}
