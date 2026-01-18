#ifndef MENU_H
#define MENU_H

#include <stdint.h>
#include <avr/pgmspace.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Public configuration */
#define MENU_MAX_ITEMS            7u
// #define MENU_MAX_ITEM_NAME_LEN    10u

#ifndef MENU_MAX_MENUS
#define MENU_MAX_MENUS 13u
#endif

/* Opaque handle: users can't access struct fields directly */
typedef struct Menu Menu_t;

/* Result codes */
typedef enum
{
    MENU_OK = 1,
    MENU_ERROR = 0
} menu_status_t;

/* Action returned by menu_enter_selected() */
typedef enum
{
    MENU_ACTION_NONE = 0,   /* No navigation happened */
    MENU_ACTION_ENTER,      /* Entered a submenu */
    MENU_ACTION_BACK,       /* Went back to previous menu */
    MENU_ACTION_SOFTKEY     /* Soft-key action handled */
} menu_action_t;

typedef void (*menu_softkey_cb_t)(void);

/* Global navigation pointers (defined in menu.c) */
extern Menu_t *g_root_menu;
extern Menu_t *g_current_menu;

/**
 * Initialize a menu.
 *
 * This function stores a PROGMEM pointer to item names.
 * - items_without_back must be <= MENU_MAX_ITEMS
 *
 * PGM_P is a typedef for const char* in PROGMEM space, not in RAM
 * because on AVR we want to save RAM.
 */
menu_status_t menu_init(Menu_t *menu,
                        uint8_t items_without_back,
                        PGM_P const *item_names);

/**
 * Create a new menu instance (from internal static storage).
 * Returns NULL if maximum number of menus is reached.
 */
Menu_t *menu_create(void);

/**
 * Set menu title/name (copied safely).
 * You can call this for any menu after menu_init().
 */
void menu_set_name(Menu_t *menu, PGM_P name);
void menu_set_soft_keys(Menu_t *menu, PGM_P const *soft_keys, const menu_softkey_cb_t *soft_actions);

/**
 * Link a submenu to a parent menu item (1-based index).
 * Valid indices: 1 .. menu_max_items.
 *
 * This also sets submenu->previous_menu = parent internally.
 */
menu_status_t menu_set_submenu(Menu_t *parent, uint8_t item_index_1based, Menu_t *submenu);

/**
 * Select an item by 1-based index.
 * Wrap behavior:
 * - if index < 1 -> wraps to last item
 * - if index > total selectable -> wraps to first item
 *
 * Returns the current selected index (1..total selectable).
 */
uint8_t menu_select(Menu_t *menu, uint8_t item_index_1based);

/* Convenience navigation */
uint8_t menu_next(Menu_t *menu);
uint8_t menu_prev(Menu_t *menu);

/**
 * "Enter" the currently selected item:
 * - If selected item is a linked submenu -> sets g_current_menu to that submenu and returns MENU_ACTION_ENTER
 * - If selected item is a soft-key -> runs its action and returns MENU_ACTION_SOFTKEY
 * - Otherwise returns MENU_ACTION_NONE
 */
menu_action_t menu_enter_selected(Menu_t *menu);
menu_status_t menu_back(Menu_t *menu);

/* Getters (read-only pointers to internal storage) */
PGM_P menu_get_name(const Menu_t *menu);
PGM_P menu_get_status_left(const Menu_t *menu);
PGM_P menu_get_status_right(const Menu_t *menu);
PGM_P menu_get_item_name(const Menu_t *menu, uint8_t item_index_1based);
PGM_P menu_get_soft_key(const Menu_t *menu, uint8_t key_pos_1based);
menu_softkey_cb_t menu_get_soft_key_action(const Menu_t *menu, uint8_t key_pos_1based);

uint8_t menu_get_max_items(const Menu_t *menu);            /* list items only */
uint8_t menu_get_selected(const Menu_t *menu);             /* 1..total selectable */
uint8_t menu_get_selected_item(const Menu_t *menu);        /* 1..list items or 0 */
uint8_t menu_get_selected_softkey_pos(const Menu_t *menu); /* 1..3 or 0 */
uint8_t menu_get_total_selectable(const Menu_t *menu);     /* list + non-empty softkeys */


#ifdef __cplusplus
}
#endif

#endif /* MENU_H */
