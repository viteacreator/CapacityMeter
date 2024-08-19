#ifndef MAIN_H
#define MAIN_H

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "INA219.h"
#include "softwareTimer.h"
//#include "btn.h"

#define RED_LED 13
#define SCREEN_ADDRESS 0x3C  // OLED display I2C address

extern Adafruit_SSD1306 display;
extern INA219 ina;

uint32_t prevLoopMillis = 0;
volatile float capacity = 0;
uint32_t prevActiveCurrMillis = 0;
uint32_t totalActiveCurrMillis = 0;
volatile uint16_t voltage = 0;
volatile int16_t current = 0;
volatile int16_t absCurrent = 0;
volatile uint32_t millisTime = 0;
uint32_t loopTime = 0;
uint32_t lastTimeExt0 = 0;
volatile bool readInaFlag = 0;
uint32_t prevTimeTest = 0;
uint32_t timeTest = 0;

extern SoftTimer displayShowTimer;
extern SoftTimer myTimer;
extern SoftTimer readInaTimer;

void setup();
void loop();
void toggleLed();
void initExtInterrupt();
void initTimer1();
void displayWrite();
void errFunc();

#endif  // MAIN_H
