#ifndef MENU_INTERNAL_H
#define MENU_INTERNAL_H

#include "menu.h"

typedef struct
{
    PGM_P prefix;
    const char *value;
} m_status_t;

typedef struct
{
    PGM_P label;
    Menu_t *submenu;
} menu_context_t;

typedef struct
{
    PGM_P label;
    menu_get_u16_fn_t get;
    menu_set_u16_fn_t set;
    uint8_t editable;
} leaf_context_t;

typedef struct
{
    PGM_P label;
    menu_softkey_cb_t cb;
    uint8_t initial_state; /* 0 = no state, 1 = first segment, etc. */
} soft_key_t;

struct Menu
{
    uint8_t selected;  /* the actual selected item, 1..total selectable */
    uint8_t menu_items_count; /* list item count */
    uint8_t leaf_items_count; /* leaf row count */
    uint8_t param_edit_flag;
    uint8_t curr_edit_row_idx;

    PGM_P menu_name;
    m_status_t status_left;
    m_status_t status_right;

    /** 
     * The context part, for menus. 
     * all items selectable and when press OK, it opens a submenu or leaf screen 
     */
    const menu_context_t *menu_rows;

    /** 
     * The context part, for leaf screens. 
     * only some items are selectable/editable,
     * others are just to show live parameters
     */
    const leaf_context_t *leaf_rows;

    /** 
     * The render function for leaf screens, because 
     * each leaf screen may have a different way to render its content.
     */
    menu_leaf_render_fn_t leaf_render;

    const soft_key_t *soft_keys;
    uint8_t soft_key_state[3];

    Menu_t *prev_menu;
};

menu_status_t menu_set_menu_rows(Menu_t *menu, uint8_t count, const menu_context_t *rows);
menu_status_t menu_set_leaf_rows(Menu_t *menu, uint8_t count, const leaf_context_t *rows);
menu_status_t menu_set_soft_keys(Menu_t *menu, const soft_key_t *keys);

#endif /* MENU_INTERNAL_H */
