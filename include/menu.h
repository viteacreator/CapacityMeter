#ifndef MENU_H
#define MENU_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Public configuration */
#define MENU_MAX_ITEMS            6u
#define MENU_MAX_ITEM_NAME_LEN    18u

#ifndef MENU_MAX_MENUS
#define MENU_MAX_MENUS 8u
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
    MENU_ACTION_BACK        /* Went back to previous menu */
} menu_action_t;

/* Global navigation pointers (defined in menu.c) */
extern Menu_t *g_root_menu;
extern Menu_t *g_current_menu;

/**
 * Initialize a menu.
 *
 * This function sets up item names and automatically appends the last item as "Back".
 * - items_without_back must be <= MENU_MAX_ITEMS - 1
 * - The last selectable item becomes "Back" (index = max_items)
 *
 * The function copies strings into internal storage.
 */
menu_status_t menu_init(Menu_t *menu,
                        uint8_t items_without_back,
                        const char *const item_names[]);

/**
 * Create a new menu instance (from internal static storage).
 * Returns NULL if maximum number of menus is reached.
 */
Menu_t *menu_create(void);

/**
 * Set menu title/name (copied safely).
 * You can call this for any menu after menu_init().
 */
void menu_set_name(Menu_t *menu, const char *name);

/**
 * Link a submenu to a parent menu item (1-based index).
 * Valid indices: 1 .. (menu_max_items - 1). The last item is "Back" and cannot have a submenu.
 *
 * This also sets submenu->previous_menu = parent internally.
 */
menu_status_t menu_set_submenu(Menu_t *parent, uint8_t item_index_1based, Menu_t *submenu);

/**
 * Select an item by 1-based index.
 * Wrap behavior:
 * - if index < 1 -> wraps to last item
 * - if index > max_items -> wraps to first item
 *
 * Returns the current selected index (1..max_items).
 */
uint8_t menu_select(Menu_t *menu, uint8_t item_index_1based);

/* Convenience navigation */
uint8_t menu_next(Menu_t *menu);
uint8_t menu_prev(Menu_t *menu);

/**
 * "Enter" the currently selected item:
 * - If selected item is a linked submenu -> sets g_current_menu to that submenu and returns MENU_ACTION_ENTER
 * - If selected item is "Back" -> sets g_current_menu to previous menu (if any) and returns MENU_ACTION_BACK
 * - Otherwise returns MENU_ACTION_NONE
 */
menu_action_t menu_enter_selected(Menu_t *menu);

/* Getters (read-only pointers to internal storage) */
const char *menu_get_name(const Menu_t *menu);
const char *menu_get_item_name(const Menu_t *menu, uint8_t item_index_1based);

uint8_t menu_get_max_items(const Menu_t *menu);          /* includes Back */
uint8_t menu_get_selected(const Menu_t *menu);           /* 1..max_items */

#ifdef __cplusplus
}
#endif

#endif /* MENU_H */
