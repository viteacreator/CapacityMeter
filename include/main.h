#ifndef MAIN_H
#define MAIN_H

#include <SPI.h>
#include <Wire.h>
// #include "INA219.h"

#include "ina_adc.h"
#include "i2c_wire_arduino.h"
#include "softwareTimer.h"
//#include "btn.h"

#include "menu.h"      /* g_current_menu */
#include "ui_tree.h"   /* UI tree + button handler */
#include "display_if.h"

#define RED_LED 13
#define SCREEN_ADDRESS 0x3C  // OLED display I2C address
// extern INA226 ina;

extern uint32_t prev_loop_millis;
extern int32_t capacity_uah;
extern uint32_t prev_active_curr_millis;
extern uint32_t total_active_curr_millis;
//volatile uint16_t voltage = 0;
//volatile int16_t current = 0;
// volatile int16_t abs_current = 0;
extern volatile uint32_t millis_time;
extern uint32_t loop_time;


//uint32_t prev_time_test = 0;
//uint32_t time_test = 0;

extern SoftTimer_t display_show_timer;
extern SoftTimer_t my_timer;
extern SoftTimer_t read_ina_timer;

void setup();
void loop();
void toggleLed();
void init_int0_interrupt();
void init_pcint_interrupts();

void initTimer1();
void displayWrite();
void errFunc();

#endif  // MAIN_H
