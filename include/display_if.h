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

bool display_if_init(void);
void display_if_begin_frame(void);
bool display_if_next_page(void);
void display_if_end_frame(void);
void display_if_clear(void);
void display_if_set_cursor(uint8_t x, uint8_t y);
void display_if_set_invert(uint8_t enable);
void display_if_print(const char *s);
void display_if_print_pgm(PGM_P s);
void display_if_println(const char *s);
void display_if_println_pgm(PGM_P s);
void display_if_newline(void);

#endif /* DISPLAY_IF_H */
