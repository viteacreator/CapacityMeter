#ifndef MAIN_H
#define MAIN_H

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
// #include "INA219.h"
#include "ina_adc.h"
#include "i2c_wire_arduino.h"
#include "softwareTimer.h"
//#include "btn.h"

#define RED_LED 13
#define SCREEN_ADDRESS 0x3C  // OLED display I2C address

extern Adafruit_SSD1306 display;
// extern INA226 ina;

uint32_t prev_loop_millis = 0;
volatile float capacity = 0;
uint32_t prev_active_curr_millis = 0;
uint32_t total_active_curr_millis = 0;
volatile uint16_t voltage = 0;
volatile int16_t current = 0;
volatile int16_t abs_current = 0;
volatile uint32_t millis_time = 0;
uint32_t loop_time = 0;
uint32_t last_time_ext0 = 0;
volatile bool read_ina_flag = 0;
uint32_t prev_time_test = 0;
uint32_t time_test = 0;

extern SoftTimer_t display_show_timer;
extern SoftTimer_t my_timer;
extern SoftTimer_t read_ina_timer;

void setup();
void loop();
void toggleLed();
void initExtInterrupt();
void initTimer1();
void displayWrite();
void errFunc();

#endif  // MAIN_H
