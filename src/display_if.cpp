#include "display_if.h"
#include "main.h"
#include <stdio.h>

// define DISPLAY_BACKEND_U8G2 - in .h file

#if defined(DISPLAY_BACKEND_U8G2)
#include <U8g2lib.h>
#elif defined(DISPLAY_BACKEND_ADAFRUIT)
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#endif

static uint8_t s_cursor_x = 0u;
static uint8_t s_cursor_y = 0u;
static uint8_t s_invert = 0u;

#if defined(DISPLAY_BACKEND_U8G2)
static U8G2_SSD1306_128X64_NONAME_1_HW_I2C s_u8g2(U8G2_R0, U8X8_PIN_NONE);
#elif defined(DISPLAY_BACKEND_ADAFRUIT)
static Adafruit_SSD1306 s_display(128, 64);
#endif

static uint8_t text_len_ram(const char *s)
{
  if (!s)
    return 0u;
  uint8_t len = 0u;
  while (s[len] != '\0')
    len++;
  return len;
}

static uint8_t text_len_pgm(PGM_P s)
{
  if (!s)
    return 0u;
  uint8_t len = 0u;
  while (pgm_read_byte(s + len) != '\0')
    len++;
  return len;
}

static void advance_cursor(uint8_t chars)
{
  s_cursor_x = (uint8_t)(s_cursor_x + (uint8_t)(chars * 6u));
}

bool display_init(void)
{
#if defined(DISPLAY_BACKEND_U8G2)
  s_u8g2.setI2CAddress((uint8_t)(SCREEN_ADDRESS << 1));
  s_u8g2.begin();
  s_u8g2.setFont(u8g2_font_5x7_tr);
  s_u8g2.setFontMode(1);
  s_u8g2.setDrawColor(1);
  return true;
#elif defined(DISPLAY_BACKEND_ADAFRUIT)
  if (!s_display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS))
  {
    return false;
  }
  s_display.clearDisplay();
  return true;
#endif
}

void display_frame_begin(void)
{
#if defined(DISPLAY_BACKEND_U8G2)
  s_u8g2.firstPage();
#endif
}

bool display_frame_next_page(void)
{
#if defined(DISPLAY_BACKEND_U8G2)
  return s_u8g2.nextPage();
#else
  return false;
#endif
}

void display_frame_end(void)
{
#if defined(DISPLAY_BACKEND_ADAFRUIT)
  s_display.display();
#endif
}

void display_clear(void)
{
#if defined(DISPLAY_BACKEND_U8G2)
  s_u8g2.clearBuffer();
#elif defined(DISPLAY_BACKEND_ADAFRUIT)
  s_display.clearDisplay();
#endif
  s_cursor_x = 0u;
  s_cursor_y = 0u;
  s_invert = 0u;
}

void display_set_cursor(uint8_t x, uint8_t y)
{
  s_cursor_x = x;
  s_cursor_y = y;
#if defined(DISPLAY_BACKEND_ADAFRUIT)
  s_display.setCursor(x, y);
#endif
}

void display_set_invert(uint8_t enable)
{
  s_invert = enable ? 1u : 0u;
#if defined(DISPLAY_BACKEND_ADAFRUIT)
  if (s_invert)
    s_display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
  else
    s_display.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
#endif
}

static void draw_text_ram(const char *s)
{
  if (!s)
    return;

  uint8_t len = text_len_ram(s);
#if defined(DISPLAY_BACKEND_U8G2)
  uint8_t w = (uint8_t)(len * 5u);
  if (s_invert)
  {
    s_u8g2.setDrawColor(1);
    s_u8g2.drawBox(s_cursor_x, s_cursor_y, w, 8u);
    s_u8g2.setDrawColor(0);
  }
  s_u8g2.setCursor(s_cursor_x, (uint8_t)(s_cursor_y + 7u));
  s_u8g2.print(s);
  if (s_invert)
    s_u8g2.setDrawColor(1);
#elif defined(DISPLAY_BACKEND_ADAFRUIT)
  s_display.print(s);
#endif
  advance_cursor(len);
}

static void draw_text_pgm(PGM_P s)
{
  if (!s)
    return;

  uint8_t len = text_len_pgm(s);
#if defined(DISPLAY_BACKEND_U8G2)
  uint8_t w = (uint8_t)(len * 5u);
  if (s_invert)
  {
    s_u8g2.setDrawColor(1);
    s_u8g2.drawBox(s_cursor_x, s_cursor_y, w, 8u);
    s_u8g2.setDrawColor(0);
  }
  s_u8g2.setCursor(s_cursor_x, (uint8_t)(s_cursor_y + 7u));
  s_u8g2.print((__FlashStringHelper *)s);
  if (s_invert)
    s_u8g2.setDrawColor(1);
#elif defined(DISPLAY_BACKEND_ADAFRUIT)
  s_display.print((__FlashStringHelper *)s);
#endif
  advance_cursor(len);
}

void display_print(const char *s)
{
  draw_text_ram(s);
}

void display_print_pgm(PGM_P s)
{
  draw_text_pgm(s);
}

void display_println(const char *s)
{
  draw_text_ram(s);
  display_newline();
}

void display_println_pgm(PGM_P s)
{
  draw_text_pgm(s);
  display_newline();
}

void display_newline(void)
{
  s_cursor_x = 0u;
  s_cursor_y = (uint8_t)(s_cursor_y + 8u);
#if defined(DISPLAY_BACKEND_ADAFRUIT)
  s_display.setCursor(s_cursor_x, s_cursor_y);
#endif
}

uint8_t display_char_width_px(void)
{
#if defined(DISPLAY_BACKEND_U8G2)
  return 5u;
#else
  return 6u;
#endif
}

/* Helper function to convert float to string */
static void format_float_to_string(float value, uint8_t decimals, char *buf, uint8_t buf_size);

/**
 * this function convert "%t" to a string "12h 34m 56s" format from a time in seconds
 */
void display_print_formated_time_pgm(PGM_P template_str, uint16_t total_seconds)
{
  if (!template_str)
    return;

  uint16_t hours = total_seconds / 3600u;
  uint16_t minutes = (total_seconds % 3600u) / 60u;
  uint16_t seconds = total_seconds % 60u;

  char val_buf[20]; // to keep the formatted time
  snprintf(val_buf, sizeof(val_buf), "%uh %um %us", hours, minutes, seconds);

  char ch;
  uint8_t i = 0u;
  while ((ch = pgm_read_byte(template_str + i)) != '\0')
  {
    if (ch == '%' &&
        (pgm_read_byte(template_str + i + 1u) == 't'))
    {
      /* Found %t placeholder, insert the formatted time */
      display_print(val_buf);
      i += 2u;
    }
    else
    {
      /* Regular character, print it */
      char buf[2] = {ch, '\0'};
      display_print(buf);
      i++;
    }
  }
}

void display_print_formatted_u16_pgm(PGM_P template_str, uint16_t value)
{
  if (!template_str)
    return;

  char val_buf[8]; // to keep the value. 8 is enough for uint16_t
  snprintf(val_buf, sizeof(val_buf), "%u", value);

  char ch;
  uint8_t i = 0u;
  while ((ch = pgm_read_byte(template_str + i)) != '\0')
  {
    if (ch == '%' &&
        (pgm_read_byte(template_str + i + 1u) == 'i'))
    {
      /* Found %i placeholder, insert the value */
      display_print(val_buf);
      i += 2u;
    }
    else
    {
      /* Regular character, print it */
      char buf[2] = {ch, '\0'};
      display_print(buf);
      i++;
    }
  }
}

void display_print_formatted_float_pgm(PGM_P template_str, float value)
{
  if (!template_str)
    return;

  char ch;
  uint8_t i = 0u;

  while ((ch = pgm_read_byte(template_str + i)) != '\0')
  {
    if (ch == '%')
    {
      /* Check for float format specifiers: %f, %.1f, %.2f, etc. */
      char next = pgm_read_byte(template_str + i + 1u);
      uint8_t decimals = 6u; /* Default decimal places */

      if (next == 'f')
      {
        /* %f format (default 6 decimal places) */
        char val_buf[20];
        format_float_to_string(value, decimals, val_buf, sizeof(val_buf));
        display_print(val_buf);
        i += 2u;
      }
      else if (next == '.')
      {
        /* Check for %.Nf format where N is a digit */
        char digit_ch = pgm_read_byte(template_str + i + 2u);
        char f_ch = pgm_read_byte(template_str + i + 3u);

        if (digit_ch >= '0' && digit_ch <= '9' && f_ch == 'f')
        {
          /* Found %.Nf format */
          decimals = (uint8_t)(digit_ch - '0');
          char val_buf[20];
          format_float_to_string(value, decimals, val_buf, sizeof(val_buf));
          display_print(val_buf);
          i += 4u;
        }
        else
        {
          /* Not a valid format, print the character as-is */
          char buf[2] = {ch, '\0'};
          display_print(buf);
          i++;
        }
      }
      else
      {
        /* Not a float format, print the character as-is */
        char buf[2] = {ch, '\0'};
        display_print(buf);
        i++;
      }
    }
    else
    {
      /* Regular character, print it */
      char buf[2] = {ch, '\0'};
      display_print(buf);
      i++;
    }
  }
}

static void format_float_to_string(float value, uint8_t decimals, char *buf, uint8_t buf_size)
{
  if (!buf || buf_size < 2)
    return;

  uint8_t pos = 0u;
  
  /* Handle negative numbers */
  if (value < 0.0f)
  {
    if (pos < buf_size - 1)
      buf[pos++] = '-';
    value = -value;
  }

  /* Get integer part */
  uint32_t int_part = (uint32_t)value;
  float frac_part = value - (float)int_part;

  /* Convert integer part to string */
  char int_buf[16];
  uint8_t int_len = 0u;
  if (int_part == 0u)
  {
    int_buf[int_len++] = '0';
  }
  else
  {
    uint32_t temp = int_part;
    while (temp > 0u && int_len < sizeof(int_buf))
    {
      int_buf[int_len++] = (char)('0' + (temp % 10u));
      temp /= 10u;
    }
  }

  /* Copy integer part (reversed) */
  for (uint8_t i = 0u; i < int_len && pos < buf_size - 1; i++)
  {
    buf[pos++] = int_buf[int_len - 1u - i];
  }

  /* Add decimal point if needed */
  if (decimals > 0u && pos < buf_size - 1)
  {
    buf[pos++] = '.';
  }

  /* Convert fractional part */
  for (uint8_t i = 0u; i < decimals && pos < buf_size - 1; i++)
  {
    frac_part *= 10.0f;
    uint8_t digit = (uint8_t)frac_part;
    buf[pos++] = (char)('0' + digit);
    frac_part -= (float)digit;
  }

  buf[pos] = '\0';
}

void display_print_smart_pgm(PGM_P template_str, uint16_t value_u16, float value_float)
{
  if (!template_str)
    return;

  /* Scan the string to detect which format specifier is used */
  uint8_t i = 0u;
  char format_type = 0; /* 0=none, 'i'=%i, 'f'=%f, 't'=%t */

  while (pgm_read_byte(template_str + i) != '\0')
  {
    char ch = pgm_read_byte(template_str + i);
    if (ch == '%')
    {
      char next = pgm_read_byte(template_str + i + 1u);
      if (next == 'i')
      {
        format_type = 'i';
        break;
      }
      else if (next == 't')
      {
        format_type = 't';
        break;
      }
      else if (next == 'f' || next == '.')
      {
        format_type = 'f';
        break;
      }
    }
    i++;
  }

  /* Call the appropriate formatter based on detected format */
  switch (format_type)
  {
    case 'i':
      display_print_formatted_u16_pgm(template_str, value_u16);
      break;
    case 't':
      display_print_formated_time_pgm(template_str, value_u16);
      break;
    case 'f':
      display_print_formatted_float_pgm(template_str, value_float);
      break;
    default:
      /* No recognized format, just print the template */
      display_print_pgm(template_str);
      break;
  }
}
