#include "main.h"

#define THRESHOLD_DW_HIGH 3000 // Upper discharge voltage threshold (3.0V)
// #define THRESHOLD_DW_LOW 2800   // Lower voltage threshold (2.8V)
// #define THRESHOLD_UP_HIGH 4200  // Upper voltage threshold (4.2V)
#define THRESHOLD_UP_LOW 4190 // Lower charging voltage threshold (4.19V)
#define RELAYPIN 3
#define HISTPERIOD 3000

Adafruit_SSD1306 display(128, 64);
INA226 ina((uint8_t)0x40);

bool relay_state = 0;
uint32_t hist_time_elapse = 0;

// Pin pin = (Pin){ &PORTB, PB1 };
// Btn btn;

SoftTimer display_show_timer;
SoftTimer my_timer;
SoftTimer read_ina_timer;

//----------------------------------------------------------------------------------------------------------------------------------------------------------
void setup()
{
  initExtInterrupt();
  initTimer1();

  // Initialize the software timers
  soft_timer_init(&my_timer, 100, NULL);             // 100 ms interval
  soft_timer_init(&display_show_timer, 300, NULL);    // 300 ms interval
  soft_timer_init(&read_ina_timer, 100, computeData); // 100 ms interval

  // Start the software timers
  soft_timer_start(&my_timer);
  soft_timer_start(&display_show_timer);
  soft_timer_start(&read_ina_timer);

  // initBtn(&btn, &pin);

  Serial.begin(9600);
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS))
  {
    Serial.println(F("SSD1306 allocation failed"));
    errFunc();
  }
  // Clear the display buffer
  display.clearDisplay();

  if (ina.begin())
  {
    Serial.println(F("INA226 connected!"));
  }
  else
  {
    Serial.println(F("INA226 not found!"));
    errFunc();
  }

  Serial.print(F("Calibration value: "));
  Serial.println(ina.getCalibration());
  // ina.setCalibVolt(1.0059f);                                // Set calibration voltage
  ina.adjCalibration(27);                             // Adjust calibration
  ina.setSampleTime(INA226_VBUS, INA226_AVG_X1024);   // Set bus voltage resolution
  ina.setSampleTime(INA226_VSHUNT, INA226_AVG_X1024); // Set shunt voltage resolution

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
void loop()
{
  uint32_t actmillis_time = millisT();

  if (time_elapsed_flag(&read_ina_timer))
  {
    /* to do INA226 reading in C style

// Instead of C++:
// INA226 ina(0.1f, 0.8f, 0x40);
// ina.begin();
// float voltage = ina.getVoltage();

// Now in C:
INA226_t ina;
ina226_init(&ina, 0.1f, 0.8f, 0x40);
ina226_begin(&ina);
float voltage = ina226_get_voltage(&ina);
*/
    voltage = ina.getMiliVoltage();
    current = ina.getMiliCurrent();
    abs_current = abs(current);
  }
  hystereis_relay_control(voltage, abs_current);
  digitalWrite(RELAYPIN, relay_state);

  // cli();
  //  loop_time = actmillis_time - prev_loop_millis;
  //  if (loop_time > 0) {
  //    prev_loop_millis = actmillis_time;
  //    float tempCapacity = (float)abs_current * ((float)loop_time / 3600000);
  //    capacity += tempCapacity;
  //  }
  // sei();

  if (time_elapsed_flag(&display_show_timer))
  {
    // toggleLed();
    prev_time_test = actmillis_time;
    displayWrite();
    time_test = actmillis_time - prev_time_test;
    // toggleLed();
  }

  if (abs_current > 1)
  {
    total_active_curr_millis += actmillis_time - prev_active_curr_millis;
  }
  prev_active_curr_millis = actmillis_time;
}

void computeData()
{
  uint32_t actmillis_time = millisT();
  loop_time = actmillis_time - prev_loop_millis;
  prev_loop_millis = actmillis_time;
  float tempCapacity = (float)abs_current * ((float)loop_time / 3600000);
  capacity += tempCapacity;
}
//----------------------------------------------------------------------------------------------------------------------------------------------------------
void toggleLed()
{
  PORTB ^= (1 << PB5);
  // toggleLed();
}

//----------------------------------------------------------------------------------------------------------------------------------------------------------
uint32_t millisT()
{
  return millis_time;
}

//----------------------------------------------------------------------------------------------------------------------------------------------------------
// Interrupt Service Routine for INT0
ISR(INT0_vect)
{
  if (millis_time - last_time_ext0 > 100)
  {
    last_time_ext0 = millis_time;
    PORTB ^= (1 << PB5); // Toggle PB5
    relay_state = true;
  }
}

//----------------------------------------------------------------------------------------------------------------------------------------------------------

bool hystereis_relay_control(uint16_t volt, int16_t curr)
{
  // if (relay_state && (volt > THRESHOLD_UP_HIGH || volt < THRESHOLD_DW_LOW)) {                                                                      //0v'-'-'-'l------l-------------l----l'-'-'-'..
  if (hist_time_elapse > HISTPERIOD && relay_state && (volt > (int32_t)(THRESHOLD_UP_LOW + 10 + curr / 10) || volt < (int32_t)(THRESHOLD_DW_HIGH - 100 - curr / 4)))
  {                       // 0v'-'-'-'l------l-------------l----l'-'-'-'..
    relay_state = false;  //
    hist_time_elapse = 0; //
  }
  else if (hist_time_elapse > HISTPERIOD && !relay_state && volt < THRESHOLD_UP_LOW && volt > THRESHOLD_DW_HIGH)
  {                     // 0v-------l------l'-'-'-'-'-'-'l----l------..
    relay_state = true; //
    hist_time_elapse = 0;
  }
  return relay_state;
}

//----------------------------------------------------------------------------------------------------------------------------------------------------------
void initExtInterrupt()
{
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
ISR(TIMER1_COMPA_vect)
{
  millis_time++;
  hist_time_elapse++;
  // PORTB ^= (1 << PB5);

  soft_timer_update(&my_timer);
  soft_timer_update(&display_show_timer);
  soft_timer_update(&read_ina_timer);

  // cccvCompute();
}

//----------------------------------------------------------------------------------------------------------------------------------------------------------
void initTimer1()
{
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
void displayWrite()
{
  display.clearDisplay();
  display.setTextSize(2);              // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE); // Draw white text

  display.setCursor(22, 0); // Start at top-left corner
  display.print(voltage);
  display.println("mV");

  if (current < 0)
  {
    display.setCursor(10, 16);
  }
  else
  {
    display.setCursor(22, 16);
  }
  display.print(current);
  display.println("mA");

  display.setTextSize(1); // Normal 1:1 pixel scale
  display.setCursor(0, 32);
  display.print("Act:");
  display.print(total_active_curr_millis / 1000);
  display.println("s");

  display.setCursor(60, 32);
  display.print("Cap:");
  display.print((uint16_t)capacity);
  display.println("mAh");

  display.setCursor(0, 40);
  display.print("All:");
  display.print(millis_time / 1000);
  display.println("s");

  display.setCursor(0, 48);
  display.print("Lt:");
  display.print(loop_time);
  display.println("ms");

  display.setCursor(60, 48);
  display.print("Tt:");
  display.print(time_test);
  display.println("ms");

  display.display();
}

//----------------------------------------------------------------------------------------------------------------------------------------------------------
void errFunc()
{
  for (int i = 0; i < 10; i++)
  {
    digitalWrite(RED_LED, LOW);
    delay(250);
    digitalWrite(RED_LED, HIGH);
    delay(650);
  }
}
