# STM32-WATCH

A programmable smart watch based on the STM32F103C8T6 microcontroller, featuring an OLED display, real-time clock, motion sensing, and multiple interactive apps.

## Features

- **Clock** — RTC-backed digital clock with date/time setting menu
- **Stopwatch** — Lap timer with start/stop/reset
- **Pedometer** — Step counter using MPU6050 accelerometer
- **Thermometer** — NTC thermistor temperature sensor (YL-38 module, PA0)
- **Dino Game** — Side-scrolling obstacle game with score tracking
- **Flashlight** — LED toggle
- **Level** — Tilt sensor using MPU6050 gyroscope/accelerometer
- **Animated emoji** — Display animations

## Hardware

| Component | Description |
|-----------|-------------|
| MCU | STM32F103C8T6 (Blue Pill, 72 MHz Cortex-M3) |
| Display | 0.96" SSD1306 OLED 128x64, I2C (SCL=PB8, SDA=PB9) |
| IMU | MPU6050 6-axis accelerometer/gyroscope, I2C |
| RTC | STM32 internal RTC with backup battery |
| Thermometer | NTC thermistor (YL-38 module) on PA0 via ADC |
| Battery | 302530 Li-Po, TP4056 charger IC, ME6211C33 3.3 V LDO |
| Buttons | 3x tactile buttons |
| Size | 39.5 mm x 45.5 mm x 14.5 mm |

<img width="1280" height="1706" alt="48ff4a5b0389d6fd098f9042dba37e4d" src="https://github.com/user-attachments/assets/5a2a519f-a917-4673-922f-d622e0778159" />
<img width="1280" height="1706" alt="a57737a8d27008fa6db3bfe8c1006309" src="https://github.com/user-attachments/assets/f0bf1f54-7eed-4594-bc0e-1905b9557acf" />

## Repository Structure

```
KeilCode/code/
  Hardware/   — Peripheral drivers (OLED, MPU6050, RTC, thermistor, menu, dino game…)
    System/     — Delay, RTC helpers, timer
      User/       — main.c, interrupt handlers, peripheral config
        Library/    — STM32F10x Standard Peripheral Library
          Start/      — CMSIS startup files and core_cm3
          hardware/     — PCB schematics and 3D enclosure models
          ```

          ## Build

          Open `KeilCode/code/Project.uvprojx` in Keil MDK µVision5 (ARMCC v5). Build and flash via ST-LINK.

          ## Wiring (thermistor add-on)

          Connect the YL-38 NTC thermistor module: VCC → 3.3 V, GND → GND, AO → PA0.
          
