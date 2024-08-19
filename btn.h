#ifndef BTN_H
#define BTN_H

#include "hallGPIO.h"
#include <avr/io.h>
#include <stdbool.h>



typedef struct {
  Pin *_pin;
  uint32_t _tmr;
  uint32_t _tmr_hold;
  bool _flag;
} Btn;

void initBtn(Btn *btn, Pin *pin) {
  *btn = {0};
  btn->_pin = pin;
  initPinAsInput(pin);
}

bool click() {
  bool btnState = digitalRead(Btn->_pin);
  if (!btnState && !Btn->_flag && millis() - Btn->_tmr >= 100) {
    Btn->_flag = true;
    Btn->_tmr = millis();
    return true;
  }
  // if (!btnState && _flag && millis() - _tmr >= 100) { // If butt is press more than 100ms...
  //   _tmr_hold = millis();
  //   return true;                                                                            //... return true every hold_delay ms
  // }
  if (btnState && Btn->_flag) {
    Btn->_flag = false;
    Btn->_tmr = millis();
  }
  return false;
}
bool check(uint8_t hold_delay) {
  bool btnState = digitalRead(_pin);
  if (!btnState && !_flag && millis() - _tmr >= 100) {
    _flag = true;
    _tmr = millis();
    return true;
  }
  if (!btnState && _flag && millis() - _tmr >= 500 && millis() - _tmr_hold > hold_delay) {  // If butt is press more than 500ms...
    _tmr_hold = millis();
    return true;  //... return true every hold_delay ms
  }
  if (btnState && _flag) {
    _flag = false;
    _tmr = millis();
  }
  return false;
}

#endif  // BTN_H