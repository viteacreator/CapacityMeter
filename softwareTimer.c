#include "softwareTimer.h"

/*
 * Initialize the software timer module
 *
 * Sets up the timer structure with the specified interval and callback function.
 *
 * @param timer - Pointer to timer structure
 * @param intervalMs - Interval in milliseconds
 * @param callback - Callback function pointer (NULL for no callback)
 */
void soft_timer_init(SoftTimer_t *timer, uint32_t interval_ms, void (*callback)(void))
{
    timer->interval_ms = interval_ms; /* Set the interval for the timer */
    timer->elapsed_time = 0;          /* Initialize elapsed time to 0 */
    timer->elapsed_time_flag = false; /* Clear the elapsed time flag */
    timer->state = TIMER_STOP;        /* Set the timer state to stopped */
    timer->callback = callback;       /* Set the callback function */
}

/*
 * Update the timer, called from the ISR every 1 ms
 *
 * This function should be called from a timer interrupt service routine
 * to increment the elapsed time and check if the interval has been reached.
 *
 * @param timer - Pointer to timer structure
 */
void soft_timer_update(SoftTimer_t *timer)
{

    if (timer->state != TIMER_START){
        return; /* If the timer is not running, exit */
    }
    timer->elapsed_time++; /* Increment the elapsed time */

    if (timer->elapsed_time >= timer->interval_ms)
    {                                    /* Check if the interval has elapsed */
        timer->elapsed_time = 0;         /* Reset the elapsed time */
        timer->elapsed_time_flag = true; /* Set the elapsed time flag */

        if (timer->callback != NULL)
        { /* If a callback function is set, call it */
            timer->callback();
        }
    }
}

/*
 * Check and reset the elapsed time flag
 *
 * Returns true if the interval has elapsed and resets the flag.
 * This function should be called in the main loop to check if the timer
 * has expired since the last check.
 *
 * @param timer - Pointer to timer structure
 * @return true if interval elapsed, false otherwise
 */
bool time_elapsed_flag(SoftTimer_t *timer)
{
    if (timer->elapsed_time_flag)
    {                                     /* If the flag is true */
        timer->elapsed_time_flag = false; /* Clear the flag */
        return true;                      /* Return true */
    }
    else
    {
        return false; /* Return false */
    }
}

/*
 * Start a specific timer
 *
 * Transitions the timer from stopped to running state and resets
 * the elapsed time counter.
 *
 * @param timer - Pointer to timer structure
 */
void soft_timer_start(SoftTimer_t *timer)
{
    timer->elapsed_time = 0;    /* Reset the elapsed time */
    timer->state = TIMER_START; /* Set the timer state to running */
}

/*
 * Stop a specific timer
 *
 * Transitions the timer from running to stopped state.
 *
 * @param timer - Pointer to timer structure
 */
void soft_timer_stop(SoftTimer_t *timer)
{
    timer->state = TIMER_STOP; /* Set the timer state to stopped */
}
