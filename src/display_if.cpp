#include "display_if.h"
#include "main.h"

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

bool display_if_init(void)
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

void display_if_begin_frame(void)
{
#if defined(DISPLAY_BACKEND_U8G2)
  s_u8g2.firstPage();
#endif
}

bool display_if_next_page(void)
{
#if defined(DISPLAY_BACKEND_U8G2)
  return s_u8g2.nextPage();
#else
  return false;
#endif
}

void display_if_end_frame(void)
{
#if defined(DISPLAY_BACKEND_ADAFRUIT)
  s_display.display();
#endif
}

void display_if_clear(void)
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

void display_if_set_cursor(uint8_t x, uint8_t y)
{
  s_cursor_x = x;
  s_cursor_y = y;
#if defined(DISPLAY_BACKEND_ADAFRUIT)
  s_display.setCursor(x, y);
#endif
}

void display_if_set_invert(uint8_t enable)
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

void display_if_print(const char *s)
{
  draw_text_ram(s);
}

void display_if_print_pgm(PGM_P s)
{
  draw_text_pgm(s);
}

void display_if_println(const char *s)
{
  draw_text_ram(s);
  display_if_newline();
}

void display_if_println_pgm(PGM_P s)
{
  draw_text_pgm(s);
  display_if_newline();
}

void display_if_newline(void)
{
  s_cursor_x = 0u;
  s_cursor_y = (uint8_t)(s_cursor_y + 8u);
#if defined(DISPLAY_BACKEND_ADAFRUIT)
  s_display.setCursor(s_cursor_x, s_cursor_y);
#endif
}
