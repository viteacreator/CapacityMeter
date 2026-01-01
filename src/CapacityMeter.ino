#include "main.h"

/* Buttons are INPUT_PULLUP -> pressed = LOW */
/** todo: its better in future to use:
 * PD2(PCINT18/INT0), PD4(PCINT20/T0), PB0(PCINT0)
 * because there is no peripheral on these pins needed in this prj
 */
#define BTN_PLUS_PIN 7  // Pin for PLUS button, PD7 (PCINT23)
#define BTN_OK_PIN 2    // Pin for OK button, PD2 (INT0)
#define BTN_MINUS_PIN 9 // Pin for MINUS button, PB1 (PCINT1)
// #define BTN_OK_PIN     8 // Pin for OK button, PB0 (PCINT0)

#define BTN_DEBOUNCE_MS 100u // Button debounce time in milliseconds

#define THRESHOLD_DW_HIGH 3000 // Upper discharge voltage threshold (3.0V)
// #define THRESHOLD_DW_LOW 2800   // Lower voltage threshold (2.8V)
// #define THRESHOLD_UP_HIGH 4200  // Upper voltage threshold (4.2V)
#define THRESHOLD_UP_LOW 4190 // Lower charging voltage threshold (4.19V)
#define RELAYPIN 3
#define HISTPERIOD 3000

Adafruit_SSD1306 display(128, 56);
// INA226 ina((uint8_t)0x40);
INA226_t ina;

bool relay_state = 0;
uint32_t hist_time_elapse = 0;

// Pin pin = (Pin){ &PORTB, PB1 };
// Btn btn;

SoftTimer_t display_show_timer;
SoftTimer_t my_timer;
SoftTimer_t read_ina_timer;

static void ui_render_menu(Menu_t *m);
void computeData();

/*------------------------------------------------------------------*/
/* main functions                                                   */
/*------------------------------------------------------------------*/
void setup()
{ 
  
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  // here we have to init the display on its own buffer, not from our ram because it is eating half of the ram
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS))
  {
    Serial.println(F("SSD1306 allocation failed"));
    errFunc();
  }
  // Clear the display buffer
  display.clearDisplay();



  init_int0_interrupt();
  init_pcint_interrupts();
  initTimer1();

  // Initialize the software timers
  soft_timer_init(&my_timer, 100, NULL);           // 100 ms interval
  soft_timer_init(&display_show_timer, 300, NULL); // 300 ms interval
  soft_timer_init(&read_ina_timer, 100, NULL);     // 100 ms interval
  // Start the software timers
  soft_timer_start(&my_timer);
  soft_timer_start(&display_show_timer);
  soft_timer_start(&read_ina_timer);

  // initMenus();
  // initBtn(&btn, &pin);

  ina226_init(&ina, 0.1f, 3.2f, 0x40);      // Initialize with shunt resistance, max current, I2C address
  ina226_bind_i2c(&ina, &I2C_WIRE_ARDUINO); // Attach I2C backend

  ui_tree_init(); // Initialize UI tree

  Serial.begin(115200);

  if (ina226_begin(&ina))
  {
    Serial.println(F("INA226 connected!"));
  }
  else
  {
    Serial.println(F("INA226 not found!"));
    errFunc();
  }

  Serial.print(F("Calibration value: "));
  Serial.println(ina226_get_calibration(&ina));
  // ina.setCalibVolt(1.0059f);                                // Set calibration voltage
  ina226_adj_calibration(&ina, 27);                              // Adjust calibration
  ina226_set_sample_time(&ina, INA226_VBUS, INA226_AVG_X1024);   // Set bus voltage resolution
  ina226_set_sample_time(&ina, INA226_VSHUNT, INA226_AVG_X1024); // Set shunt voltage resolution
  // Initialize the LED pin as an output
  pinMode(RED_LED, OUTPUT);
  // pinMode(7, INPUT_PULLUP);
  // pinMode(8, INPUT_PULLUP);
  // pinMode(9, INPUT_PULLUP);
  // Set PB5 (Pin 13 on Arduino Uno) as output
  DDRB |= (1 << PB5);
  DDRD |= (1 << PD3); // for relay
}
//-----------------------------------------------------------------------------
void loop()
{
  uint32_t actmillis_time = millisT();

  if (time_elapsed_flag(&read_ina_timer))
  {
    voltage = ina226_get_mili_voltage(&ina);
    current = ina226_get_mili_current(&ina);
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
    // displayWrite();
    ui_render_menu(g_current_menu);
    time_test = actmillis_time - prev_time_test;
    // toggleLed();
  }

  if (time_elapsed_flag(&read_ina_timer))
  {
    computeData();
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

  float tempCapacity = (float)abs_current * ((float)loop_time / 3600000); // mAh calculation 
  capacity += tempCapacity; // accumulate capacity in mAh
}

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

/*------------------------------------------------------------------*/
/* isr and timers                                                   */
/*------------------------------------------------------------------*/

void init_int0_interrupt()
{
  // Configure PD2 (INT0) Ard pin 2 on arduino, as an input
  DDRD &= ~(1 << PD2);
  // Enable pull-up resistor on PD2
  PORTD |= (1 << PD2);
  // Set the interrupt sense control to trigger on a falling edge
  EICRA |= (1 << ISC01) | (0 << ISC00);
  // Enable INT0 interrupt
  EIMSK |= (1 << INT0);
  // Enable global interrupts
  sei();
}
// Interrupt Service Routine for INT0 from external pin (PD2)
ISR(INT0_vect)
{
  if ((millis_time - last_time_btn_ok) < BTN_DEBOUNCE_MS)
    return;
  if (PIND & (1 << PD2))
    return;

  last_time_btn_ok = millis_time;
  PORTB ^= (1 << PB5); // Toggle PB5, Ard pin 13
  relay_state = true;

  /* in future need to use a g_ok_event flag to not overload isr */
  ui_on_button(UI_BTN_OK); // Handle OK button press for display
}
//-----------------------------------------------------------------------------
void init_pcint_interrupts()
{
  // Configure PB1 (PCINT1), and PD7 (PCINT23) as inputs (without PB0 because it is conn to other pin now)
  DDRB &= ~(1 << PB1);
  DDRD &= ~(1 << PD7);
  // Enable pull-up resistor on PB1 and PD7
  PORTB |= (1 << PB1);
  PORTD |= (1 << PD7);
  // Enable pin change interrupt for PCINT1 (PB1), PCINT23 (PD7)
  PCICR |= (1 << PCIE0) | (1 << PCIE2); // Enable PCINT0x and PCINT2x groups for PB1 and PD7
  // Enable PCINT interrupt
  PCMSK0 |= (1 << PCINT1);  // Enable PCINT1 for PB1
  PCMSK2 |= (1 << PCINT23); // Enable PCINT23 for PD7
  // Enable global interrupts
  sei();
}

ISR(PCINT0_vect)
{
  static uint8_t prev_pinb = 0xFF;               /* previous sampled PINB state */
  uint8_t pinb = PINB;                           /* current PINB state */
  uint8_t changed = (uint8_t)(pinb ^ prev_pinb); /* changed bits */

  /* Debounce time gate */
  if ((millis_time - last_time_btn_dw) < BTN_DEBOUNCE_MS)
  {
    prev_pinb = pinb;
    return;
  }

  if (changed & (1 << PB1))
  { /* Check if PB1 changed (not other one in group)*/
    /* Falling edge: was HIGH, now LOW */
    if ((prev_pinb & (1 << PB1)) && !(pinb & (1 << PB1)))
    {
      last_time_btn_dw = millis_time;
      ui_on_button(UI_BTN_MINUS);
    }
  }
  prev_pinb = pinb;
}
ISR(PCINT2_vect)
{
  static uint8_t prev_pind = 0xFF;
  uint8_t pind = PIND;
  uint8_t changed = (uint8_t)(pind ^ prev_pind);

  if ((millis_time - last_time_btn_up) < BTN_DEBOUNCE_MS)
  {
    prev_pind = pind;
    return;
  }

  if (changed & (1 << PD7))
  { /* Check if PD7 changed (not other pin in group) */
    if ((prev_pind & (1 << PD7)) && !(pind & (1 << PD7)))
    { /* check for falling edge */
      last_time_btn_up = millis_time;
      ui_on_button(UI_BTN_PLUS);
    }
  }
  prev_pind = pind;
}

//-----------------------------------------------------------------------------
void initTimer1() // Initialize Timer1 for 1 ms interrupts
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

uint32_t millisT(void)
{
  uint32_t t;
  uint8_t sreg = SREG;
  cli();
  t = millis_time;
  SREG = sreg;
  return t;
}

/*------------------------------------------------------------------*/
/* new display write                                                */
/*------------------------------------------------------------------*/
static void oled_print_pgm(PGM_P s)
{
  char buf[20];
  if (!s)
  {
    display.println(F("NULL"));
    return;
  }
  strncpy_P(buf, s, sizeof(buf) - 1u);
  buf[sizeof(buf) - 1u] = '\0';
  display.println(buf);
}
static void ui_render_menu(Menu_t *m)
{
  display.clearDisplay();
  display.setTextSize(1);

  // title
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);

  // const char *title = menu_get_name(m);
  // display.println(title ? title : "NULL");
  // display.println("----------------");
  oled_print_pgm(menu_get_name(m));

  uint8_t max = menu_get_max_items(m);
  uint8_t sel = menu_get_selected(m);

  // for (uint8_t i = 1u; i <= max; i++)
  // {
  //   if (i == sel)
  //     display.print("> ");
  //   else
  //     display.print("  ");

  //   const char *name = menu_get_item_name(m, i);
  //   display.println(name ? name : "");
  // }
  for (uint8_t i = 1u; i <= max; i++)
  {
    // if (display.print((i == sel)))
    if (i == sel)
    {
      // display.setTextColor(SSD1306_WHITE, SSD1306_BLACK); // Inverted color for selected item
      display.print(F("> "));
    }
    else
    {
      // display.setTextColor(SSD1306_BLACK, SSD1306_WHITE); // Normal color for other items
      display.print(F("  "));
    }
    oled_print_pgm(menu_get_item_name(m, i));
  }

  display.display();
}

/*------------------------------------------------------------------*/
/* old display write                                                */
/*------------------------------------------------------------------*/
// void displayWrite()
// {
//   display.clearDisplay();
//   display.setTextSize(2);              // Normal 1:1 pixel scale
//   display.setTextColor(SSD1306_WHITE); // Draw white text

//   display.setCursor(22, 0); // Start at top-left corner
//   display.print(voltage);
//   display.println("mV");

//   if (current < 0)
//   {
//     display.setCursor(10, 16);
//   }
//   else
//   {
//     display.setCursor(22, 16);
//   }
//   display.print(current);
//   display.println("mA");

//   display.setTextSize(1); // Normal 1:1 pixel scale
//   display.setCursor(0, 32);
//   display.print("Act:");
//   display.print(total_active_curr_millis / 1000);
//   display.println("s");

//   display.setCursor(60, 32);
//   display.print("Cap:");
//   display.print((uint16_t)capacity);
//   display.println("mAh");

//   display.setCursor(0, 40);
//   display.print("All:");
//   display.print(millis_time / 1000);
//   display.println("s");

//   display.setCursor(0, 48);
//   display.print("Lt:");
//   display.print(loop_time);
//   display.println("ms");

//   display.setCursor(60, 48);
//   display.print("Tt:");
//   display.print(time_test);
//   display.println("ms");

//   display.display();
// }

//-----------------------------------------------------------------------------
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

void toggleLed()
{
  PORTB ^= (1 << PB5);
  // toggleLed();
}
