#include <stdint.h>

struct Button {
  uint8_t _pin;
  
  uint32_t _tmr;
  uint32_t _tmr_hold;
  uint8_t _flag;
};

void initBtn(struct Button *currButt, uint8_t pin) {
  currButt->_pin = pin;
  pinMode(currButt->_pin, INPUT_PULLUP);
}

bool click(struct Button *currButt) {
  bool btnState = digitalRead(currButt->_pin);
  if (!btnState && !currButt->_flag && millis() - currButt->_tmr >= 100) {
    currButt->_flag = true;
    currButt->_tmr = millis();
    return true;
  }
  if (!btnState && currButt->_flag && millis() - currButt->_tmr >= 100) {  // If butt is pressed more than 100ms...
    currButt->_tmr_hold = millis();
    return true;  //... return true
  }
  if (btnState && currButt->_flag) {
    currButt->_flag = false;
    currButt->_tmr = millis();
  }
  return false;
}

bool check(struct Button *currButt, uint8_t hold_delay) {
  bool btnState = digitalRead(currButt->_pin);
  if (!btnState && !currButt->_flag && millis() - currButt->_tmr >= 100) {
    currButt->_flag = true;
    currButt->_tmr = millis();
    return true;
  }
  if (!btnState && currButt->_flag && millis() - currButt->_tmr >= 100 && millis() - currButt->_tmr_hold > hold_delay) {  // If butt is press more than 500ms...
    currButt->_tmr_hold = millis();
    return true;  //... return true every hold_delay ms
  }
  if (btnState && currButt->_flag) {
    currButt->_flag = false;
    currButt->_tmr = millis();
  }
  return false;
}

// bool check(struct Button *currButt) {
//   return check(currButt0);
// }