#ifndef UI_TREE_H
#define UI_TREE_H

#include <stdint.h>
#include "menu.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    UI_BTN_PLUS = 0,
    UI_BTN_OK,
    UI_BTN_MINUS
} ui_button_t;

/* Build all screens/menus and link the tree */
void ui_tree_init(void);

/* Feed button events here */
void ui_on_button(ui_button_t btn);
/* Render the current menu/screen */
//void ui_render_current(void);

#ifdef __cplusplus
}
#endif

#endif /* UI_TREE_H */