#include "main.h"

#define THRESHOLD_DW_HIGH 3000  // Pragul superior de tensiune descarcare (3.0V)
// #define THRESHOLD_DW_LOW 2800   // Pragul inferior de tensiune (2.8V)
// #define THRESHOLD_UP_HIGH 4200  // Pragul superior de tensiune (4.2V)
#define THRESHOLD_UP_LOW 4190  // Pragul inferior de tensiune incarcare (4.19V)
#define RELAYPIN 3

Adafruit_SSD1306 display(128, 64);
INA219 ina(INA219_MAX_16V, 0x40);

bool relay_state = 0;

//Pin pin = (Pin){ &PORTB, PB1 };
//Btn btn;

SoftTimer displayShowTimer;
SoftTimer myTimer;
SoftTimer readInaTimer;

//----------------------------------------------------------------------------------------------------------------------------------------------------------
void setup() {
  initExtInterrupt();
  initTimer1();

  // Initialize the software timers
  softTimerInit(&myTimer, 100, NULL);              // 100 ms interval
  softTimerInit(&displayShowTimer, 300, NULL);     // 300 ms interval
  softTimerInit(&readInaTimer, 100, computeData);  // 100 ms interval

  // Start the software timers
  softTimerStart(&myTimer);
  softTimerStart(&displayShowTimer);
  softTimerStart(&readInaTimer);


  //initBtn(&btn, &pin);

  Serial.begin(9600);
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    errFunc();
  }
  // Clear the display buffer
  display.clearDisplay();

  if (ina.begin()) {
    Serial.println(F("INA219 connected!"));
  } else {
    Serial.println(F("INA219 not found!"));
    errFunc();
  }

  Serial.print(F("Calibration value: "));
  Serial.println(ina.getCalibration());
  ina.setCalibVolt(1.0059f);                                // Set calibration voltage
  ina.adjCalibration(27);                                   // Adjust calibration
  ina.setResolution(INA219_VBUS, INA219_RES_12BIT_X128);    // Set bus voltage resolution
  ina.setResolution(INA219_VSHUNT, INA219_RES_12BIT_X128);  // Set shunt voltage resolution

  // Initialize the LED pin as an output
  pinMode(RED_LED, OUTPUT);
  pinMode(7, INPUT_PULLUP);
  pinMode(8, INPUT_PULLUP);
  pinMode(9, INPUT_PULLUP);
  // Set PB5 (Pin 13 on Arduino Uno) as output
  DDRB |= (1 << PB5);
  DDRD |= (1 << PD3);
}
//----------------------------------------------------------------------------------------------------------------------------------------------------------
void loop() {
  uint32_t actMillisTime = millisT();

  if (timeElapsedFlag(&readInaTimer)) {
    voltage = ina.getMiliVoltage();
    current = ina.getMiliCurrent();
    absCurrent = abs(current);
  }
  hystereis_relay_control(voltage, absCurrent);
  digitalWrite(RELAYPIN, relay_state);

  //cli();
  // loopTime = actMillisTime - prevLoopMillis;
  // if (loopTime > 0) {
  //   prevLoopMillis = actMillisTime;
  //   float tempCapacity = (float)absCurrent * ((float)loopTime / 3600000);
  //   capacity += tempCapacity;
  // }
  //sei();

  if (timeElapsedFlag(&displayShowTimer)) {
    // toggleLed();
    prevTimeTest = actMillisTime;
    displayWrite();
    timeTest = actMillisTime - prevTimeTest;
    // toggleLed();
  }


  if (absCurrent > 1) {
    totalActiveCurrMillis += actMillisTime - prevActiveCurrMillis;
  }
  prevActiveCurrMillis = actMillisTime;
}

void computeData() {
  uint32_t actMillisTime = millisT();
  loopTime = actMillisTime - prevLoopMillis;
  prevLoopMillis = actMillisTime;
  float tempCapacity = (float)absCurrent * ((float)loopTime / 3600000);
  capacity += tempCapacity;
}
//----------------------------------------------------------------------------------------------------------------------------------------------------------
void toggleLed() {
  PORTB ^= (1 << PB5);
  // toggleLed();
}

//----------------------------------------------------------------------------------------------------------------------------------------------------------
uint32_t millisT() {
  return millisTime;
}

//----------------------------------------------------------------------------------------------------------------------------------------------------------
// Interrupt Service Routine for INT0
ISR(INT0_vect) {
  if (millisTime - lastTimeExt0 > 100) {
    lastTimeExt0 = millisTime;
    PORTB ^= (1 << PB5);  // Toggle PB5
    relay_state = true;
  }
}

//----------------------------------------------------------------------------------------------------------------------------------------------------------

bool hystereis_relay_control(uint16_t volt, int16_t curr) {
  // if (relay_state && (volt > THRESHOLD_UP_HIGH || volt < THRESHOLD_DW_LOW)) {                                     //0v'-'-'-'l------l-------------l----l'-'-'-'..
  if (relay_state && (volt > (THRESHOLD_UP_LOW + 10 + curr / 10) || volt < (THRESHOLD_DW_HIGH - 100 - curr / 4))) {  //0v'-'-'-'l------l-------------l----l'-'-'-'..
    relay_state = false;                                                                                             //
  } else if (!relay_state && volt < THRESHOLD_UP_LOW && volt > THRESHOLD_DW_HIGH) {                                  //0v-------l------l'-'-'-'-'-'-'l----l------..
    relay_state = true;                                                                                              //
  }
  return relay_state;
}

//----------------------------------------------------------------------------------------------------------------------------------------------------------
void initExtInterrupt() {
  // Configure INT0 (PD2) as an input
  DDRD &= ~(1 << PD2);
  // Enable pull-up resistor on PD2 (optional)
  PORTD |= (1 << PD2);
  // Set the interrupt sense control to trigger on a falling edge
  EICRA |= (1 << ISC01) | (0 << ISC00);
  // Enable INT0 interrupt
  EIMSK |= (1 << INT0);
  // Enable global interrupts
  sei();
}

//----------------------------------------------------------------------------------------------------------------------------------------------------------
// Timer1 Compare Match A Interrupt Service Routine
ISR(TIMER1_COMPA_vect) {
  millisTime++;
  //PORTB ^= (1 << PB5);

  softTimerUpdate(&myTimer);
  softTimerUpdate(&displayShowTimer);
  softTimerUpdate(&readInaTimer);

  // cccvCompute();
}

//----------------------------------------------------------------------------------------------------------------------------------------------------------
void initTimer1() {
  // Ensure Timer1 is in a known state
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1 = 0;
  // Set Timer1 to CTC mode
  TCCR1B |= (1 << WGM12);
  // Set prescaler to 64 (16 MHz / 64 = 250 kHz)
  TCCR1B |= (1 << CS11) | (1 << CS10);
  // Calculate and set OCR1A for 1 ms interrupt
  OCR1A = 249;
  // Enable Timer1 Compare Match A interrupt
  TIMSK1 |= (1 << OCIE1A);
  // Enable global interrupts
  sei();
}

//----------------------------------------------------------------------------------------------------------------------------------------------------------
void displayWrite() {
  display.clearDisplay();
  display.setTextSize(2);               // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE);  // Draw white text

  display.setCursor(22, 0);  // Start at top-left corner
  display.print(voltage);
  display.println("mV");

  if (current < 0) {
    display.setCursor(10, 16);
  } else {
    display.setCursor(22, 16);
  }
  display.print(current);
  display.println("mA");

  display.setTextSize(1);  // Normal 1:1 pixel scale
  display.setCursor(0, 32);
  display.print("Act:");
  display.print(totalActiveCurrMillis / 1000);
  display.println("s");

  display.setCursor(60, 32);
  display.print("Cap:");
  display.print((uint16_t)capacity);
  display.println("mAh");

  display.setCursor(0, 40);
  display.print("All:");
  display.print(millisTime / 1000);
  display.println("s");

  display.setCursor(0, 48);
  display.print("Lt:");
  display.print(loopTime);
  display.println("ms");

  display.setCursor(60, 48);
  display.print("Tt:");
  display.print(timeTest);
  display.println("ms");

  display.display();
}

//----------------------------------------------------------------------------------------------------------------------------------------------------------
void errFunc() {
  for (int i = 0; i < 10; i++) {
    digitalWrite(RED_LED, LOW);
    delay(250);
    digitalWrite(RED_LED, HIGH);
    delay(650);
  }
}
