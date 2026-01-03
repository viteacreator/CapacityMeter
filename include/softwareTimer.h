#ifndef SOFT_TIMER_H
#define SOFT_TIMER_H

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Enumeration for timer states */
typedef enum
{
  TIMER_STOP = 0x00, /* Timer is stopped */
  TIMER_START = 0x01 /* Timer is running */
} TimerState_t;

/* Timer structure */
typedef struct
{
  uint32_t interval_ms;   /* Timer interval when is triggered */
  uint32_t elapsed_time;  /* Elapsed time */
  bool elapsed_time_flag; /* Flag set when the interval has elapsed */
  TimerState_t state;     /* Current state of the timer (stopped or running) */
  void (*callback)(void); /* Callback function to be called when the interval has elapsed */
} SoftTimer_t;

/* Function Declarations */

/*
 * Initialize the software timer module
 *
 * @param timer - Pointer to timer structure
 * @param intervalMs - Interval in milliseconds
 * @param callback - Callback function pointer (NULL for no callback)
 */
void soft_timer_init(SoftTimer_t *timer, uint32_t interval_ms, void (*callback)(void));

/*
 * Update the timer, called from the ISR every 1 ms
 *
 * @param timer - Pointer to timer structure
 */
void soft_timer_update(SoftTimer_t *timer);

/*
 * Check and reset the elapsed time flag
 *
 * @param timer - Pointer to timer structure
 * @return true if interval elapsed, false otherwise
 */
bool time_elapsed_flag(SoftTimer_t *timer);

/*
 * Start a specific timer
 *
 * @param timer - Pointer to timer structure
 */
void soft_timer_start(SoftTimer_t *timer);

/*
 * Stop a specific timer
 *
 * @param timer - Pointer to timer structure
 */
void soft_timer_stop(SoftTimer_t *timer);

#ifdef __cplusplus
}
#endif

#endif /* SOFT_TIMER_H */
