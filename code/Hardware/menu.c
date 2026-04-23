#include "stm32f10x.h"                  // Device header
#include "OLED.h"
#include "MyRTC.h"
#include "Key.h"
#include "LED.h"
#include "SetTime.h"
#include "menu.h"
/* #include "MPU6050.h" */ /* Uncomment when MPU6050 module is present */
#include "Delay.h"
#include <math.h>
#include "dino.h"
/* #include "AD.h" */ /* Uncomment when ADC battery sensing is needed */
#include "Thermistor.h"


uint8_t KeyNum;	// Stores the current key value
double pi = 3.14159265358979; /* dino.c uses this */

/**
  * Function : Peripheral initialization
  * Arguments: none
  * Returns  : none
  * Note     : Initializes all on-board peripherals (RTC, keys, LED, etc.)
  *            then proceeds with the main application loop.
  */
void Peripheral_Init(void)
{
	MyRTC_Init();
	Key_Init();
	LED_Init();
	Thermistor_Init();     /* Initialize ADC for thermistor on PA0 */
	/* MPU6050_Init(); */  /* Disabled — no MPU6050 module fitted */
}



/*----------------------------------Home Clock Page-------------------------------------*/

uint16_t ADValue;
float VBAT;
int Battery_Capacity;

/* Show battery icon and percentage */
void Show_Battery(void)
{
	int sum;
	for(int i=0;i<3000;i++)
	{
		ADValue=AD_GetValue();
		sum+=ADValue;

	}
	ADValue=sum/3000;
	VBAT=(float)ADValue/4095*3.3;
	Battery_Capacity=(ADValue-3276)*100/819;
	if(Battery_Capacity<0)Battery_Capacity=0;

	//OLED_ShowNum(64,0,ADValue,4,OLED_6X8);
	//OLED_Printf(64,8,OLED_6X8,"VBAT:%.2f",VBAT);
	OLED_ShowNum(85,4,Battery_Capacity,3,OLED_6X8);
	OLED_ShowChar(103,4,'%',OLED_6X8);

	if(Battery_Capacity==100)OLED_ShowImage(110,0,16,16,Battery);
	else if(Battery_Capacity>=10&&Battery_Capacity<100)
	{
		OLED_ShowImage(110,0,16,16,Battery);
		OLED_ClearArea((112+Battery_Capacity/10),5,(10-Battery_Capacity/10),6);
		OLED_ClearArea(85,4,6,8);
	}

	else
	{
		OLED_ShowImage(110,0,16,16,Battery);
		OLED_ClearArea(112,5,10,6);
		OLED_ClearArea(85,4,12,8);
	}
};


/* Render the home clock screen */
void Show_Clock_UI(void)
{
	/* Show_Battery(); */ /* Disabled — no ADC module; re-enable when battery sensing is available */
	MyRTC_ReadTime();
	OLED_Printf(0,0,OLED_6X8,"%d-%d-%d",MyRTC_Time[0],MyRTC_Time[1],MyRTC_Time[2]);
	OLED_Printf(16,16,OLED_12X24,"%02d:%02d:%02d",MyRTC_Time[3],MyRTC_Time[4],MyRTC_Time[5]);
	OLED_ShowString(0,48,"Menu",OLED_8X16);
	OLED_ShowString(96,48,"Set",OLED_8X16);
}

int clkflag=1;	// Home clock cursor flag; default 1 = first option selected

/* Handle cursor movement and selection on the home clock page */
int First_Page_Clock(void)
{
	while(1)
	{
		KeyNum=Key_GetNum();

		if(KeyNum==1)		// Previous option
		{
			clkflag--;
			if(clkflag<=0)clkflag=2;
		}
		else if(KeyNum==2)	// Next option
		{
			clkflag++;
			if(clkflag>=3)clkflag=1;
		}
		else if(KeyNum==3)	// Confirm
		{
			OLED_Clear();
			OLED_Update();
			return clkflag;
		}

		/* KeyNum==4: power button on PCB — implement as needed */
		switch(clkflag)
		{
			case 1:
				Show_Clock_UI();
				OLED_ReverseArea(0,48,32,16);
				OLED_Update();
				break;

			case 2:
				Show_Clock_UI();
				OLED_ReverseArea(96,48,32,16);
				OLED_Update();
				break;
		}
	}
}

/*----------------------------------Settings Page-------------------------------------*/

/* Render the settings page UI */
void Show_SettingPage_UI(void)
{
	OLED_ShowImage(0,0,16,16,Return);
	OLED_ShowString(0,16,"Set Time...",OLED_8X16);
}

int setflag=1;
/* Handle cursor movement and selection on the settings page */
int SettingPage(void)
{
	while(1)
	{
		KeyNum=Key_GetNum();
		uint8_t setflag_temp=0;
		if(KeyNum==1)		// Previous option
		{
			setflag--;
			if(setflag<=0)setflag=2;
		}
		else if(KeyNum==2)	// Next option
		{
			setflag++;
			if(setflag>=3)setflag=1;
		}
		else if(KeyNum==3)	// Confirm
		{
			OLED_Clear();
			OLED_Update();
			setflag_temp=setflag;
		}

		if(setflag_temp==1){return 0;}
		else if(setflag_temp==2){SetTime();}	// Jump to time-setting page

		switch(setflag)
		{
			case 1:
				Show_SettingPage_UI();
				OLED_ReverseArea(0,0,16,16);
				OLED_Update();
				break;

			case 2:
				Show_SettingPage_UI();
				OLED_ReverseArea(0,16,96,16);
				OLED_Update();
				break;
		}
	}
}

/*----------------------------------Main Menu (icon carousel)-------------------------------------*/

uint8_t pre_selection;		// Previously selected menu item
uint8_t target_selection;	// Target selected menu item
uint8_t x_pre=48;			// X coordinate of previously selected item
uint8_t Speed=4;			// Scroll speed (pixels per frame)
uint8_t move_flag;			// 1 = animation in progress, 0 = stopped


/* Render one frame of the menu scroll animation */
void Menu_Animation(void)
{
	OLED_Clear();
	OLED_ShowImage(42,10,44,44,Frame);

	if(pre_selection<target_selection)
	{
		x_pre-=Speed;
		if(x_pre==0)
		{
			pre_selection++;
			move_flag=0;
			x_pre=48;
		}
	}

	if(pre_selection>target_selection)
	{
		x_pre+=Speed;
		if(x_pre==96)
		{
			pre_selection--;
			move_flag=0;
			x_pre=48;
		}
	}

	if(pre_selection>=1)
	{
		OLED_ShowImage(x_pre-48,16,32,32,Menu_Graph[pre_selection-1]);
	}

	if(pre_selection>=2)
	{
		OLED_ShowImage(x_pre-96,16,32,32,Menu_Graph[pre_selection-2]);
	}

	OLED_ShowImage(x_pre,16,32,32,Menu_Graph[pre_selection]);
	OLED_ShowImage(x_pre+48,16,32,32,Menu_Graph[pre_selection+1]);
	OLED_ShowImage(x_pre+96,16,32,32,Menu_Graph[pre_selection+2]);

	OLED_Update();
}


/* Set selection target and trigger animation */
void Set_Selection(uint8_t move_flag,uint8_t Pre_Selection,uint8_t Target_Selection)
{
	if(move_flag==1)
	{
		pre_selection=Pre_Selection;
		target_selection=Target_Selection;

	}
	Menu_Animation();
}

/* Scroll all icons upward off-screen (transition animation into a function) */
void MenuToFunction(void)
{
	for(uint8_t i=0;i<=6;i++)	// 7 iterations push icons off the top of the screen
	{
		OLED_Clear();
			if(pre_selection>=1)
		{
			OLED_ShowImage(x_pre-48,16+8*i,32,32,Menu_Graph[pre_selection-1]);
		}


		OLED_ShowImage(x_pre,16+8*i,32,32,Menu_Graph[pre_selection]);
		OLED_ShowImage(x_pre+48,16+8*i,32,32,Menu_Graph[pre_selection+1]);

		OLED_Update();
	}

}


uint8_t menu_flag=1;
/* Handle icon carousel navigation and function dispatch */
int Menu(void)
{
	move_flag=1;
	uint8_t DirectFlag=2;	// 1 = scrolling left, 2 = scrolling right
	while(1)
	{
		KeyNum=Key_GetNum();
		uint8_t menu_flag_temp=0;
		if(KeyNum==1)		// Previous item
		{
			DirectFlag=1;
			move_flag=1;
			menu_flag--;
			if(menu_flag<=0)menu_flag=6; /* 6 menu items total */
		}
		else if(KeyNum==2)	// Next item
		{
			DirectFlag=2;
			move_flag=1;
			menu_flag++;
			if(menu_flag>=7)menu_flag=1; /* 6 menu items total */
		}
		else if(KeyNum==3)	// Confirm
		{
			OLED_Clear();
			OLED_Update();
			menu_flag_temp=menu_flag;
		}

		if(menu_flag_temp==1){return 0;}                               // Back
		else if(menu_flag_temp==2){MenuToFunction();StopWatch();}    // Stopwatch
		else if(menu_flag_temp==3){MenuToFunction();LED();}          // LED flashlight
		else if(menu_flag_temp==4){MenuToFunction();Game();}         // Mini game
		else if(menu_flag_temp==5){MenuToFunction();Emoji();}        // Animated emoji
		else if(menu_flag_temp==6){MenuToFunction();Temperature();}  // Temperature
		/* MPU6050() and Gradienter() removed — no MPU6050 module fitted */


			if(menu_flag==1)
			{
				if(DirectFlag==1)Set_Selection(move_flag,1,0);
				else if(DirectFlag==2)Set_Selection(move_flag,0,0);
			}

			else
			{
				if(DirectFlag==1)Set_Selection(move_flag,menu_flag,menu_flag-1);
				else if(DirectFlag==2)Set_Selection(move_flag,menu_flag-2,menu_flag-1);
			}
			/* menu_flag range: 1~6; pre_selection range: 0~5 */
			/* Menu_Graph[0~7]: indices 0~5 are real icons, 6~7 are blank placeholders (for edge scrolling) */
	}
}

/*----------------------------------Stopwatch-------------------------------------*/

uint8_t hour,min,sec;
/* Render the stopwatch screen */
void Show_StopWatch_UI(void)
{
	OLED_ShowImage(0,0,16,16,Return);
	OLED_Printf(32,20,OLED_8X16,"%02d:%02d:%02d",hour,min,sec);
	OLED_ShowString(0,44,"Start",OLED_8X16);
		OLED_ShowString(48,44,"Stop",OLED_8X16);
		OLED_ShowString(88,44,"Reset",OLED_8X16);
}

uint8_t start_timing_flag;	// 1 = counting, 0 = paused

/* Called from TIM2 IRQ every 1ms — advances stopwatch counter */
void StopWatch_Tick(void)
{
	static uint16_t Count;
	Count++;
	if(Count>=1000)
	{
		Count=0;
			if(start_timing_flag==1)
		{
			sec++;
			if(sec>=60)
			{
				sec=0;
				min++;
				if(min>=60)
				{
					min=0;
					hour++;
					if(hour>99)hour=0;
				}
			}
		}
	}

}


uint8_t stopwatch_flag=1;
/* Handle cursor movement and selection on the stopwatch page */
int StopWatch(void)
{
	while(1)
	{
		KeyNum=Key_GetNum();
		uint8_t stopwatch_flag_temp=0;
		if(KeyNum==1)		// Previous option
		{
			stopwatch_flag--;
			if(stopwatch_flag<=0)stopwatch_flag=4;
		}
		else if(KeyNum==2)	// Next option
		{
			stopwatch_flag++;
			if(stopwatch_flag>=5)stopwatch_flag=1;
		}
		else if(KeyNum==3)	// Confirm
		{
			OLED_Clear();
			OLED_Update();
			stopwatch_flag_temp=stopwatch_flag;
		}

		if(stopwatch_flag_temp==1){return 0;}


		switch(stopwatch_flag)
		{
			case 1:
				Show_StopWatch_UI();
				OLED_ReverseArea(0,0,16,16);
				OLED_Update();
				break;

			case 2:
				Show_StopWatch_UI();
				start_timing_flag=1;
				OLED_ReverseArea(8,44,32,16);
				OLED_Update();
				break;

			case 3:
				Show_StopWatch_UI();
				start_timing_flag=0;
				OLED_ReverseArea(48,44,32,16);
				OLED_Update();
				break;

			case 4:
				Show_StopWatch_UI();
				start_timing_flag=0;
				hour=min=sec=0;
				OLED_ReverseArea(88,44,32,16);
				OLED_Update();
				break;
		}
	}
}

/*----------------------------------LED Flashlight-------------------------------------*/

/* Render the LED control screen */
void Show_LED_UI(void)
{
	OLED_ShowImage(0,0,16,16,Return);
	OLED_ShowString(20,20,"OFF",OLED_12X24);
	OLED_ShowString(72,20,"ON",OLED_12X24);
}

uint8_t led_flag=1;
/* Handle cursor movement and selection on the LED control page */
int LED(void)
{
	while(1)
	{
		KeyNum=Key_GetNum();
		uint8_t led_flag_temp=0;
		if(KeyNum==1)		// Previous option
		{
			led_flag--;
			if(led_flag<=0)led_flag=3;
		}
		else if(KeyNum==2)	// Next option
		{
			led_flag++;
			if(led_flag>=4)led_flag=1;
		}
		else if(KeyNum==3)	// Confirm
		{
			OLED_Clear();
			OLED_Update();
			led_flag_temp=led_flag;
		}

		if(led_flag_temp==1){return 0;}


		switch(led_flag)
		{
			case 1:
				Show_LED_UI();
				OLED_ReverseArea(0,0,16,16);
				OLED_Update();
				break;

			case 2:
				Show_LED_UI();
				LED_OFF();
				OLED_ReverseArea(20,20,36,24);
				OLED_Update();
				break;

			case 3:
				Show_LED_UI();
				LED_ON();
				OLED_ReverseArea(72,20,24,24);
				OLED_Update();
				break;


		}
	}
}

/*----------------------------------MPU6050 (disabled)-------------------------------------*/
/* MPU6050 module not fitted. Clock and level functions can be re-enabled by
   uncommenting the MPU6050 include and the relevant code blocks below. */

/* When MPU6050 is available, restore the following globals and functions:

int16_t ax,ay,az,gx,gy,gz;
float roll_g,pitch_g,yaw_g;
float roll_a,pitch_a;
float Roll,Pitch,Yaw;
float a=0.9;
float Delta_t=0.005;
double pi=3.1415927;

void MPU6050_Calculation(void) { ... }
void Show_MPU6050_UI(void) { ... }
int MPU6050(void) { ... }

*/

/*----------------------------------Mini Game-------------------------------------*/

void Show_Game_UI(void)
{
	OLED_ShowImage(0,0,16,16,Return);
		OLED_ShowString(0,16,"Snake Game",OLED_8X16);
}

uint8_t game_flag=1;
/* Handle cursor movement and selection on the game selection page */
int Game(void)
{
	while(1)
	{
		KeyNum=Key_GetNum();
		uint8_t game_flag_temp=0;
		if(KeyNum==1)		// Previous option
		{
			game_flag--;
			if(game_flag<=0)game_flag=2;
		}
		else if(KeyNum==2)	// Next option
		{
			game_flag++;
			if(game_flag>=3)game_flag=1;
		}
		else if(KeyNum==3)	// Confirm
		{
			OLED_Clear();
			OLED_Update();
			game_flag_temp=game_flag;
		}

		if(game_flag_temp==1){return 0;}
		else if(game_flag_temp==2){DinoGame_Pos_Init();DinoGame_Animation();}

		switch(game_flag)
		{
			case 1:
				Show_Game_UI();
				OLED_ReverseArea(0,0,16,16);
				OLED_Update();
				break;

			case 2:
				Show_Game_UI();
				LED_OFF();
				OLED_ReverseArea(0,16,80,16);
				OLED_Update();
				break;



		}
	}
}

/*----------------------------------Animated Emoji-------------------------------------*/

/* Render one frame of the animated emoji (blink animation) */
void Show_Emoji_UI(void)
{
	/* Close eyes */
	for(uint8_t i=0;i<3;i++)
	{
		OLED_Clear();
		OLED_ShowImage(30,10+i,16,16,Eyebrow[0]);	// Left eyebrow
		OLED_ShowImage(82,10+i,16,16,Eyebrow[1]);	// Right eyebrow
		OLED_DrawEllipse(40,32,6,6-i,1);			// Left eye
		OLED_DrawEllipse(88,32,6,6-i,1);			// Right eye
		OLED_ShowImage(54,40,20,20,Mouth);
		OLED_Update();
		Delay_ms(100);
	}

	/* Open eyes */
	for(uint8_t i=0;i<3;i++)
	{
		OLED_Clear();
		OLED_ShowImage(30,12-i,16,16,Eyebrow[0]);	// Left eyebrow
		OLED_ShowImage(82,12-i,16,16,Eyebrow[1]);	// Right eyebrow
		OLED_DrawEllipse(40,32,6,4+i,1);			// Left eye
		OLED_DrawEllipse(88,32,6,4+i,1);			// Right eye
		OLED_ShowImage(54,40,20,20,Mouth);
		OLED_Update();
		Delay_ms(100);
	}

	Delay_ms(500);

}

/* Press any confirm key to exit the emoji animation */
int Emoji(void)
{
	while(1)
	{
		KeyNum=Key_GetNum();
		if(KeyNum==3)
		{
			OLED_Clear();
			OLED_Update();
			return 0;
		}

		Show_Emoji_UI();

	}
}

/*----------------------------------Level / Gradienter (disabled)-------------------------------------*/
/* Gradienter depends on MPU6050 — disabled. Re-enable by uncommenting the MPU6050 include. */

/*----------------------------------Temperature-------------------------------------*/

/* Render the temperature display screen */
void Show_Temperature_UI(void)
{
	float temp = Get_Temperature();

	OLED_ShowImage(0, 0, 16, 16, Return);

	/* Title */
	OLED_ShowString(20, 0, "Temp", OLED_8X16);

	/* Display integer and decimal parts separately (no %f in OLED_Printf on all compilers) */
	int16_t t_int  = (int16_t)temp;
	int16_t t_dec  = (int16_t)((temp - t_int) * 10);
	if (t_dec < 0) t_dec = -t_dec;

	OLED_ShowSignedNum(8,  24, t_int, 3, OLED_12X24);   /* e.g. +026 */
	OLED_ShowChar(56, 24, '.', OLED_12X24);
	OLED_ShowNum(68,  24, t_dec, 1, OLED_12X24);         /* one decimal place */
	OLED_ShowString(80, 24, "C", OLED_12X24);
}

/* Press key3 to exit the temperature page */
int Temperature(void)
{
	while (1)
	{
		KeyNum = Key_GetNum();
		if (KeyNum == 3)
		{
			OLED_Clear();
			OLED_Update();
			return 0;
		}

		OLED_Clear();
		Show_Temperature_UI();
		OLED_Update();
	}
}
