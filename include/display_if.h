#ifndef DISPLAY_IF_H
#define DISPLAY_IF_H

#include <stdint.h>
#include <avr/pgmspace.h>

/* Backend selection (define one in build flags or before including this header) */
/* #define DISPLAY_BACKEND_ADAFRUIT */
#define DISPLAY_BACKEND_U8G2

#if defined(DISPLAY_BACKEND_ADAFRUIT) && defined(DISPLAY_BACKEND_U8G2)
#error "Define only one of DISPLAY_BACKEND_ADAFRUIT or DISPLAY_BACKEND_U8G2"
#endif

#if !defined(DISPLAY_BACKEND_ADAFRUIT) && !defined(DISPLAY_BACKEND_U8G2)
#define DISPLAY_BACKEND_ADAFRUIT
#endif

bool display_init(void);
void display_frame_begin(void);
bool display_frame_next_page(void);
void display_frame_end(void);
void display_clear(void);
void display_set_cursor(uint8_t x, uint8_t y);
void display_set_invert(uint8_t enable);
void display_print(const char *s);
void display_print_pgm(PGM_P s);
void display_println(const char *s);
void display_println_pgm(PGM_P s);
void display_newline(void);
uint8_t display_char_width_px(void);

#endif /* DISPLAY_IF_H */
