#include "main.h"
#include "button_input.h"

/* Buttons are INPUT_PULLUP -> pressed = LOW */
/** todo: its better in future to use:
 * PD2(PCINT18/INT0), PD4(PCINT20/T0), PB0(PCINT0)
 * because there is no peripheral on these pins needed in this prj
 */
#define BTN_PLUS_PIN 7  // Pin for PLUS button, PD7 (PCINT23)
#define BTN_OK_PIN 2    // Pin for OK button, PD2 (INT0)
#define BTN_MINUS_PIN 9 // Pin for MINUS button, PB1 (PCINT1)
// #define BTN_OK_PIN     8 // Pin for OK button, PB0 (PCINT0)


#define THRESHOLD_DW_HIGH 3000 // Upper discharge voltage threshold (3.0V)
// #define THRESHOLD_DW_LOW 2800   // Lower voltage threshold (2.8V)
// #define THRESHOLD_UP_HIGH 4200  // Upper voltage threshold (4.2V)
#define THRESHOLD_UP_LOW 4190 // Lower charging voltage threshold (4.19V)
#define RELAYPIN 3
#define HISTPERIOD 3000

// INA226 ina((uint8_t)0x40);
INA226_t ina;

bool relay_state = 0;
uint16_t hist_time_elapse = 0;

uint32_t prev_loop_millis = 0;
int32_t capacity_uah = 0;
uint32_t prev_active_curr_millis = 0;
uint32_t total_active_curr_millis = 0;
volatile uint32_t millis_time = 0;
uint32_t loop_time = 0;

// Pin pin = (Pin){ &PORTB, PB1 };
// Btn btn;

SoftTimer_t display_show_timer;
SoftTimer_t my_timer;
SoftTimer_t read_ina_timer;

static void button_fire(ui_button_t btn)
{
  if (btn == UI_BTN_OK)
  {
    PORTB ^= (1 << PB5); // Toggle PB5, Ard pin 13
    relay_state = true;
  }
  ui_on_button(btn);
}

static void ui_render_menu(Menu_t *m);
void computeData(int16_t abs_current);

/*------------------------------------------------------------------*/
/* main functions                                                   */
/*------------------------------------------------------------------*/
void setup()
{ 
  
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if (!display_if_init())
  {
    Serial.println(F("SSD1306 allocation failed"));
    errFunc();
  }
  display_if_begin_frame();
  do
  {
    display_if_clear();
  } while (display_if_next_page());
  display_if_end_frame();



  init_int0_interrupt();
  init_pcint_interrupts();
  initTimer1();
  buttons_init();

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
  uint16_t voltage = 0;
  int16_t current = 0;
  int16_t abs_current = 0;

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
  //    int32_t delta_uah = (int32_t)abs_current * (int32_t)loop_time / 3600;
  //    capacity_uah += delta_uah;
  //  }
  // sei();

  if (time_elapsed_flag(&display_show_timer))
  {
    // toggleLed();
    //prev_time_test = actmillis_time;
    // displayWrite();
    ui_render_menu(g_current_menu);
    //time_test = actmillis_time - prev_time_test;
    // toggleLed();
  }

  if (time_elapsed_flag(&read_ina_timer))
  {
    computeData(abs_current);
  }

  buttons_poll(button_fire);

  if (abs_current > 1)
  {
    total_active_curr_millis += actmillis_time - prev_active_curr_millis;
  }
  prev_active_curr_millis = actmillis_time;
}

void computeData(int16_t abs_current)
{
  uint32_t actmillis_time = millisT();
  loop_time = actmillis_time - prev_loop_millis;
  prev_loop_millis = actmillis_time;

  int32_t delta_uah = (int32_t)abs_current * (int32_t)loop_time / 3600; // uAh
  capacity_uah += delta_uah;
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
}
// Interrupt Service Routine for INT0 from external pin (PD2)
ISR(INT0_vect)
{
  /* handled by 1ms sampler */
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
}

ISR(PCINT0_vect)
{
  /* handled by 1ms sampler */
}
ISR(PCINT2_vect)
{
  /* handled by 1ms sampler */
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

  buttons_tick_1ms();

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
  if (!s)
  {
    display_if_println_pgm(PSTR("NULL"));
    return;
  }
  display_if_println_pgm(s);
}
static uint8_t oled_copy_pgm(PGM_P s, char *buf, uint8_t buf_size)
{
  if (buf == 0 || buf_size == 0u)
  {
    return 0u;
  }
  if (!s)
  {
    buf[0] = '\0';
    return 0u;
  }
  uint8_t i = 0u;
  for (; (i + 1u) < buf_size; i++)
  {
    char c = (char)pgm_read_byte(s + i);
    if (c == '\0')
    {
      break;
    }
    buf[i] = c;
  }
  buf[i] = '\0';
  return i;
}
static uint8_t oled_len_pgm(PGM_P s, uint8_t max_len)
{
  if (!s || max_len == 0u)
  {
    return 0u;
  }
  uint8_t i = 0u;
  for (; i < max_len; i++)
  {
    char c = (char)pgm_read_byte(s + i);
    if (c == '\0')
    {
      break;
    }
  }
  return i;
}
static void ui_render_soft_menu(Menu_t *m)
{
  const uint8_t max = menu_get_max_items(m);
  if (max == 0u)
  {
    if (menu_get_total_selectable(m) == 0u)
    {
      return;
    }
  }

  /* Skip root menu to keep the bottom line clean */
  if (m == g_root_menu)
  {
    return;
  }

  char buf[12];
  PGM_P left_p = menu_get_soft_key(m, 1u);
  PGM_P center_p = menu_get_soft_key(m, 2u);
  PGM_P right_p = menu_get_soft_key(m, 3u);
  uint8_t left_len = oled_len_pgm(left_p, (uint8_t)(sizeof(buf) - 1u));
  uint8_t center_len = oled_len_pgm(center_p, (uint8_t)(sizeof(buf) - 1u));
  uint8_t right_len = oled_len_pgm(right_p, (uint8_t)(sizeof(buf) - 1u));
  uint8_t sel_pos = menu_get_selected_softkey_pos(m);

  const int16_t y = 56;

  if (left_len > 0u)
  {
    oled_copy_pgm(left_p, buf, sizeof(buf));
    display_if_set_cursor(0, (uint8_t)y);
    if (sel_pos == 1u)
      display_if_set_invert(1u);
    else
      display_if_set_invert(0u);
    display_if_print(buf);
    display_if_set_invert(0u);
  }

  int16_t right_x = 128 - (int16_t)right_len * 6;
  if (right_len > 0u)
  {
    if (right_x < 0)
    {
      right_x = 0;
    }
    oled_copy_pgm(right_p, buf, sizeof(buf));
    display_if_set_cursor((uint8_t)right_x, (uint8_t)y);
    if (sel_pos == 3u)
      display_if_set_invert(1u);
    else
      display_if_set_invert(0u);
    display_if_print(buf);
    display_if_set_invert(0u);
  }

  if (center_len > 0u)
  {
    int16_t center_x = (128 - (int16_t)center_len * 6) / 2;
    if (center_x < 0)
    {
      center_x = 0;
    }
    int16_t left_end = (int16_t)left_len * 6;
    int16_t center_end = center_x + (int16_t)center_len * 6;
    if (center_x > (left_end + 2) && center_end < (right_x - 2))
    {
      oled_copy_pgm(center_p, buf, sizeof(buf));
      display_if_set_cursor((uint8_t)center_x, (uint8_t)y);
      if (sel_pos == 2u)
        display_if_set_invert(1u);
      else
        display_if_set_invert(0u);
      display_if_print(buf);
      display_if_set_invert(0u);
    }
  }
}
static void ui_render_menu(Menu_t *m)
{
  display_if_begin_frame();
  do
  {
    display_if_clear();

    display_if_set_cursor(0, 0);
    display_if_set_invert(0u);
    oled_print_pgm(menu_get_name(m));
    if (menu_get_status_left(m) != NULL || menu_get_status_right(m) != NULL)
    {
      // todo: if this menu has states to show. maximum 2 states, one in the left, one in the right
      display_if_newline();
    }
    display_if_newline();

    uint8_t max = menu_get_max_items(m);
    uint8_t sel_item = menu_get_selected_item(m);

    const uint8_t visible = 5u;
    if (max > 0u)
    {
      uint8_t list_max = max;
      uint8_t start = 1u;
      uint8_t end = list_max;
      if (list_max > visible)
      {
        uint8_t sel_scroll = (sel_item == 0u) ? 1u : sel_item;
        if (sel_scroll <= 3u)
        {
          start = 1u;
        }
        else if (sel_scroll >= (list_max - 1u))
        {
          start = list_max - (visible - 1u);
        }
        else
        {
          start = sel_scroll - 2u;
        }
        end = start + visible - 1u;
      }
      display_if_set_cursor(0, 16);
      for (uint8_t i = start; i <= end; i++)
      {
        display_if_set_invert((i == sel_item) ? 1u : 0u);
        oled_print_pgm(menu_get_item_name(m, i));
      }
    }

    ui_render_soft_menu(m);
  } while (display_if_next_page());
  display_if_end_frame();
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
//   display.print((uint16_t)capacity_uah);
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
