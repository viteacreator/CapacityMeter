#include "button_input.h"
#include <avr/io.h>

#define BTN_DEBOUNCE_MS 30u
#define BTN_REPEAT_DELAY_MS 2000u
#define BTN_REPEAT_MS 500u

typedef struct
{
    uint8_t stable_level; /* 1 = released, 0 = pressed */
    uint8_t debounce_cnt;
    uint16_t press_start_ms;
    uint16_t last_repeat_ms;
} ButtonState_t;

typedef struct
{
    ui_button_t btn;
} ButtonEvent_t;

#define BTN_QUEUE_SIZE 8u
static volatile ButtonEvent_t s_queue[BTN_QUEUE_SIZE];
static volatile uint8_t s_q_head = 0u;
static volatile uint8_t s_q_tail = 0u;

static ButtonState_t s_ok = {1u, 0u, 0u, 0u};
static ButtonState_t s_plus = {1u, 0u, 0u, 0u};
static ButtonState_t s_minus = {1u, 0u, 0u, 0u};

static uint16_t s_tick_ms = 0u;

static uint8_t queue_push(ui_button_t btn)
{
    uint8_t next = (uint8_t)((s_q_head + 1u) % BTN_QUEUE_SIZE);
    if (next == s_q_tail)
        return 0u;
    s_queue[s_q_head].btn = btn;
    s_q_head = next;
    return 1u;
}

static uint8_t queue_pop(ButtonEvent_t *out)
{
    if (s_q_tail == s_q_head)
        return 0u;
    *out = s_queue[s_q_tail];
    s_q_tail = (uint8_t)((s_q_tail + 1u) % BTN_QUEUE_SIZE);
    return 1u;
}

static uint8_t raw_ok_level(void)
{
    return (PIND & (1 << PD2)) ? 1u : 0u;
}

static uint8_t raw_plus_level(void)
{
    return (PINB & (1 << PB1)) ? 1u : 0u;
}

static uint8_t raw_minus_level(void)
{
    return (PIND & (1 << PD7)) ? 1u : 0u;
}

static void button_tick(ButtonState_t *b, uint8_t raw_level, uint8_t allow_repeat, ui_button_t btn)
{
    if (raw_level == b->stable_level)
    {
        b->debounce_cnt = 0u;
    }
    else
    {
        if (b->debounce_cnt < (uint8_t)BTN_DEBOUNCE_MS)
            b->debounce_cnt++;
        if (b->debounce_cnt >= (uint8_t)BTN_DEBOUNCE_MS)
        {
            b->stable_level = raw_level;
            b->debounce_cnt = 0u;
            if (raw_level == 0u)
            {
                b->press_start_ms = s_tick_ms;
                b->last_repeat_ms = s_tick_ms;
                (void)queue_push(btn);
            }
        }
    }

    if (allow_repeat && b->stable_level == 0u)
    {
        if ((uint16_t)(s_tick_ms - b->press_start_ms) >= (uint16_t)BTN_REPEAT_DELAY_MS)
        {
            if ((uint16_t)(s_tick_ms - b->last_repeat_ms) >= (uint16_t)BTN_REPEAT_MS)
            {
                b->last_repeat_ms = s_tick_ms;
                (void)queue_push(btn);
            }
        }
    }
}

void buttons_init(void)
{
    s_ok.stable_level = 1u;
    s_ok.debounce_cnt = 0u;
    s_plus.stable_level = 1u;
    s_plus.debounce_cnt = 0u;
    s_minus.stable_level = 1u;
    s_minus.debounce_cnt = 0u;
}

void buttons_tick_1ms(void)
{
    s_tick_ms++;
    button_tick(&s_ok, raw_ok_level(), 0u, UI_BTN_OK);
    button_tick(&s_plus, raw_plus_level(), 1u, UI_BTN_PLUS);
    button_tick(&s_minus, raw_minus_level(), 1u, UI_BTN_MINUS);
}

void buttons_poll(button_event_cb_t cb)
{
    if (!cb)
        return;
    ButtonEvent_t ev;
    while (queue_pop(&ev))
    {
        cb(ev.btn);
    }
}
