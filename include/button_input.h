#ifndef BUTTON_INPUT_H
#define BUTTON_INPUT_H

#include <stdint.h>
#include "ui_tree.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*button_event_cb_t)(ui_button_t btn);

void buttons_init(void);
void buttons_tick_1ms(void);
void buttons_poll(button_event_cb_t cb);

#ifdef __cplusplus
}
#endif

#endif /* BUTTON_INPUT_H */
