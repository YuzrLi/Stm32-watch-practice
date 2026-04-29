# STM32-WATCH Practice

## Overview
This project is based on an open-source STM32 watch design found online, modified to fit the components I had available. Built as a personal learning project to explore STM32 hardware-software interaction.

## Hardware
- MCU: STM32F103C8T6
- Display: OLED (SSD1306, 128×64, I2C)
- Temperature sensor: NTC thermistor (YL-38 module, PA0)
- Buttons: 3
- LED: 1
- Programming/Power: ST-LINK via SWD

## Changes from Original
- Removed MPU6050-dependent features (attitude sensing, level) — module not available
- Removed battery management circuit (TP4056, ME6211C33) — powered via ST-LINK
- **Added thermometer** — reads NTC thermistor via ADC, calculates temperature using the B-parameter equation

## Features
1. Digital clock (RTC)
2. Date/time setting
3. Button-navigated scrolling icon menu
4. Stopwatch
5. LED flashlight
6. Dinosaur game
7. Animated emoji
8. Thermometer

## Development
- Keil MDK µVision5
- Flash via ST-LINK (SWD)

## Demo

**Temperature display**
<img width="1280" height="1706" alt="48ff4a5b0389d6fd098f9042dba37e4d" src="https://github.com/user-attachments/assets/f0efe2b9-c458-4cf4-af75-316165bde149" />


**Main clock**
<img width="1280" height="1706" alt="48ff4a5b0389d6fd098f9042dba37e4d" src="https://github.com/user-attachments/assets/bb19fc54-619e-46a4-8595-0a16c7cfab12" />

