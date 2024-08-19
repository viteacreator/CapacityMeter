#ifndef SOFT_TIMER_H
#define SOFT_TIMER_H

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdbool.h>

// Enumeration for timer states
typedef enum {
  timerStop = 0x00,  // Timer is stopped
  timerStart = 0x01  // Timer is running
} TimerState;

// Timer structure
typedef struct {
  uint32_t intervalMs;     // Timer interval in milliseconds
  uint32_t elapsedTime;    // Elapsed time
  bool elapsedTimeFlag;    // Flag set when the interval has elapsed
  TimerState state;        // Current state of the timer (stopped or running)
  void (*callback)(void);  // Callback function to be called when the interval has elapsed
} SoftTimer;

// Initialize the software timer module
void softTimerInit(SoftTimer *timer, uint32_t intervalMs, void (*callback)(void)) {
  timer->intervalMs = intervalMs;  // Set the interval for the timer
  timer->elapsedTime = 0;          // Initialize elapsed time to 0
  timer->elapsedTimeFlag = false;  // Clear the elapsed time flag
  timer->state = timerStop;        // Set the timer state to stopped
  timer->callback = callback;      // Set the callback function
}

// Update the timer, called from the ISR every 1 ms
void softTimerUpdate(SoftTimer *timer) {
  if (timer->state == timerStart) {                 // Check if the timer is running
    timer->elapsedTime++;                           // Increment the elapsed time
    if (timer->elapsedTime >= timer->intervalMs) {  // Check if the interval has elapsed
      timer->elapsedTime = 0;                       // Reset the elapsed time
      timer->elapsedTimeFlag = true;                // Set the elapsed time flag
      if (timer->callback) {                        // If a callback function is set, call it
        timer->callback();
      }
    }
  }
}

// Check and reset the elapsed time flag
bool timeElapsedFlag(SoftTimer *timer) {
  if (timer->elapsedTimeFlag) {      // If the flag is true
    timer->elapsedTimeFlag = false;  // Clear the flag
    return 1;                        // Return true (1)
  } else {
    return 0;  // Return false (0)
  }
}

// Start a specific timer
void softTimerStart(SoftTimer *timer) {
  timer->elapsedTime = 0;     // Reset the elapsed time
  timer->state = timerStart;  // Set the timer state to running
}

// Stop a specific timer
void softTimerStop(SoftTimer *timer) {
  timer->state = timerStop;  // Set the timer state to stopped
}

#endif  // SOFT_TIMER_H
