#ifndef __MENU_H
#define __MENU_H

void Peripheral_Init(void);
void Show_Clock_UI(void);
int  First_Page_Clock(void);
int  SettingPage(void);
int  Menu(void);
void StopWatch_Tick(void);
int  StopWatch(void);
int  LED(void);
/* int MPU6050(void); */    /* Disabled — no MPU6050 module fitted */
int  Game(void);
int  Emoji(void);
int  Temperature(void);
/* int Gradienter(void); */ /* Disabled — depends on MPU6050 */

#endif
