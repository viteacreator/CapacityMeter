#include "menu.h"
#include <string.h>
#include <avr/pgmspace.h>

/**
 * Internal menu screen definition (hidden from users).
 *
 * NOTE:
 * - On AVR we store constant strings in flash (PROGMEM) to save RAM.
 * - PGM_P is a pointer to a NUL-terminated string in flash on AVR;
 *   on non-AVR it resolves to a normal const char*.
 *
 * UI target: 128x64 SSD1306 OLED with 6x8 font.
 * Layout model: 8 text rows by 21 columns.
 *
 * The UI is a tree of Menu screens:
 * - Leaf RUN screens show live parameters and handle button actions.
 * - All other screens are menus/submenus with selectable items/submenus.
 *
 * Screen layout:
 * - Line 1: Title (menu/screen name)
 * - Line 2: Optional status line
 *   - up to two short fields (left/right), e.g. "State: progress" / "uSD:Ok"
 *   - if no status fields are provided, leave this line blank
 * - Lines 3 .. (LAST_LINE-1): Scrollable content area
 *   - list items (submenu entries) or live parameters
 *   - if items exceed visible lines, auto-scroll
 * - Line LAST_LINE: Soft-key bar (reserved)
 *   - left/center/right actions, e.g. "Back", "Exit", "Start", "Stop", "Options"
 *   - used only for context actions, not normal list items
 *   - "Back" is typically on SKL; root may omit it
 *
 * "Options" concept:
 * - Each leaf RUN screen may have a companion Options screen with extra details.
 * - This is not a generic submenu; it is a strongly related extension screen.
 */
struct Menu
{
    uint8_t selected;  /* the actual selected item, 1..total selectable */
    uint8_t max_items; /* list item count */

    PGM_P menu_name;
    // PGM_P status_left;
    // PGM_P status_right;
    PGM_P const *item_names;    /* PROGMEM array of item strings */
    PGM_P const *soft_key_bar;  /* PROGMEM array of soft key labels */
    const menu_softkey_cb_t *soft_key_actions; /* PROGMEM array of soft key actions */

    uint8_t prev_menu; /* index of the previous menu in the pool */
    uint8_t sub_menus[MENU_MAX_ITEMS];
};

/* Global pointers */
Menu_t *g_root_menu = 0;
Menu_t *g_current_menu = 0;

static Menu_t s_menu_pool[MENU_MAX_MENUS];
static uint8_t s_menu_pool_used = 0u;

#define MENU_INDEX_NONE 0xFFu

static uint8_t menu_softkey_count(const Menu_t *menu)
{
    if (menu == 0 || menu->soft_key_bar == 0)
        return 0u;

    uint8_t count = 0u;
    for (uint8_t pos = 0u; pos < 3u; pos++)
    {
        if (pgm_read_ptr(&menu->soft_key_bar[pos]) != 0)
            count++;
    }
    return count;
}

static uint8_t menu_softkey_pos_from_index(const Menu_t *menu, uint8_t index_1based)
{
    if (menu == 0 || menu->soft_key_bar == 0 || index_1based == 0u)
        return 0u;

    uint8_t count = 0u;
    for (uint8_t pos = 0u; pos < 3u; pos++)
    {
        if (pgm_read_ptr(&menu->soft_key_bar[pos]) != 0)
        {
            count++;
            if (count == index_1based)
                return (uint8_t)(pos + 1u);
        }
    }
    return 0u;
}

static uint8_t menu_index_from_ptr(const Menu_t *m)
{
    if (m == 0)
        return MENU_INDEX_NONE;
    return (uint8_t)(m - &s_menu_pool[0]);
}

static Menu_t *menu_from_index(uint8_t idx)
{
    if (idx == MENU_INDEX_NONE)
        return 0;
    return &s_menu_pool[idx];
}

Menu_t *menu_create(void)
{
    if (s_menu_pool_used >= MENU_MAX_MENUS)
        return 0;

    Menu_t *m = &s_menu_pool[s_menu_pool_used++];
    memset(m, 0, sizeof(*m));
    return m;
}

/* ---- Internal helpers ---- */

// static void safe_copy_str(char *dst, uint16_t dst_len, const char *src) //..., PGM_P src)
// {
//     /* Copies at most dst_len-1 bytes and always NUL-terminates */
//     if (dst == 0 || dst_len == 0u)
//     {
//         return;
//     }

//     if (src == 0)
//     {
//         dst[0] = '\0';
//         return;
//     }

//     strncpy(dst, src, (size_t)(dst_len - 1u));
//     dst[dst_len - 1u] = '\0';
// }

// static void safe_copy_fixed(char dst[MENU_MAX_ITEM_NAME_LEN],
//                             const char src[MENU_MAX_ITEM_NAME_LEN])
// {
//     /* Copies at most MENU_MAX_ITEM_NAME_LEN-1 and always NUL-terminates */
//     strncpy(dst, src, (size_t)(MENU_MAX_ITEM_NAME_LEN - 1u));
//     dst[MENU_MAX_ITEM_NAME_LEN - 1u] = '\0';
// }

static uint8_t clamp_wrap_1based(uint8_t value, uint8_t max_items)
{
    /* Wraps 1..max_items */
    if (max_items == 0u)
        return 0u;

    if (value < 1u)
        return max_items;
    if (value > max_items)
        return 1u;
    return value;
}

/* ---- Public API ---- */

menu_status_t menu_init(Menu_t *menu,
                        uint8_t items_without_back,
                        PGM_P const *item_names)
{
    if (menu == 0)
        return MENU_ERROR;

    if (items_without_back > MENU_MAX_ITEMS)
        return MENU_ERROR;

    if ((items_without_back > 0u) && (item_names == 0))
        return MENU_ERROR;

    /* Clear structure fields that matter */
    menu->prev_menu = MENU_INDEX_NONE;
    for (uint8_t i = 0u; i < MENU_MAX_ITEMS; i++)
    {
        menu->sub_menus[i] = MENU_INDEX_NONE;
    }

    /* Default name */
    // menu->menu_name = "Menu";
    menu->menu_name = PSTR("Menu"); // PSTR macro places string in PROGMEM

    /* Keep reference to PROGMEM item list */
    menu->item_names = item_names;
    menu->soft_key_bar = 0;
    menu->soft_key_actions = 0;

    /* Total list items */
    menu->max_items = items_without_back;

    /* Start selection at first selectable item (if any) */
    menu->selected = (menu->max_items > 0u) ? 1u : 0u;

    /* If root/current are not set, set them to the first initialized menu */
    if (g_root_menu == 0)
    {
        g_root_menu = menu;
    }
    if (g_current_menu == 0)
    {
        g_current_menu = menu;
    }

    return MENU_OK;
}

void menu_set_name(Menu_t *menu, PGM_P name)
{
    if (menu)
        menu->menu_name = name;
}

void menu_set_soft_keys(Menu_t *menu, PGM_P const *soft_keys, const menu_softkey_cb_t *soft_actions)
{
    if (menu)
    {
        menu->soft_key_bar = soft_keys;
        menu->soft_key_actions = soft_actions;

        if (menu->selected == 0u && menu_get_total_selectable(menu) > 0u)
            menu->selected = 1u;
    }
}

menu_status_t menu_set_submenu(Menu_t *parent, uint8_t item_index_1based, Menu_t *submenu)
{
    if (parent == 0 || submenu == 0)
        return MENU_ERROR;

    if (parent->max_items < 1u)
        return MENU_ERROR;

    if (item_index_1based < 1u || item_index_1based > parent->max_items)
        return MENU_ERROR;

    uint8_t submenu_idx = menu_index_from_ptr(submenu);
    uint8_t parent_idx = menu_index_from_ptr(parent);
    if ((submenu_idx == MENU_INDEX_NONE) || (parent_idx == MENU_INDEX_NONE))
        return MENU_ERROR;

    parent->sub_menus[item_index_1based - 1u] = submenu_idx;
    submenu->prev_menu = parent_idx;
    return MENU_OK;
}

uint8_t menu_select(Menu_t *menu, uint8_t item_index_1based)
{
    if (menu == 0)
        return 0u;

    uint8_t total = menu_get_total_selectable(menu);
    if (total == 0u)
    {
        /* Not initialized */
        menu->selected = 0u;
        return 0u;
    }

    menu->selected = clamp_wrap_1based(item_index_1based, total);
    return menu->selected;
}

uint8_t menu_next(Menu_t *menu)
{
    if (menu == 0)
        return 0u;
    return menu_select(menu, (uint8_t)(menu->selected + 1u));
}

uint8_t menu_prev(Menu_t *menu)
{
    if (menu == 0)
        return 0u;
    /* If selected is 1, selected-1 underflows; clamp_wrap_1based handles it */
    return menu_select(menu, (uint8_t)(menu->selected - 1u));
}

menu_action_t menu_enter_selected(Menu_t *menu)
{
    if (menu == 0 || menu->selected == 0u)
        return MENU_ACTION_NONE;

    if (menu->selected <= menu->max_items)
    {
        /* Normal item: check submenu link */
        Menu_t *next = menu_from_index(menu->sub_menus[menu->selected - 1u]);
        if (next != 0)
        {
            g_current_menu = next;
            return MENU_ACTION_ENTER;
        }
        return MENU_ACTION_NONE;
    }

    /* Soft-key action */
    uint8_t soft_index = (uint8_t)(menu->selected - menu->max_items);
    uint8_t pos = menu_softkey_pos_from_index(menu, soft_index);
    if (pos != 0u)
    {
        menu_softkey_cb_t cb = menu_get_soft_key_action(menu, pos);
        if (cb)
            cb();
    }
    return MENU_ACTION_SOFTKEY;
}

menu_status_t menu_back(Menu_t *menu)
{
    if (menu == 0)
        return MENU_ERROR;

    if (menu->prev_menu != MENU_INDEX_NONE)
    {
        g_current_menu = menu_from_index(menu->prev_menu);
        return MENU_OK;
    }
    return MENU_ERROR;
}

PGM_P menu_get_name(const Menu_t *menu)
{
    if (menu == 0)
        return 0;
    return menu->menu_name;
}

PGM_P menu_get_status_left(const Menu_t *menu)
{
    // if (menu == 0)
    //     return 0;
    // return menu->status_left;
    (void)menu;
    return 0;
}
PGM_P menu_get_status_right(const Menu_t *menu)
{
    // if (menu == 0)
    //     return 0;
    // return menu->status_right;
    (void)menu;
    return 0;
}

PGM_P menu_get_item_name(const Menu_t *menu, uint8_t item_index_1based)
{
    if (menu == 0 || menu->max_items == 0u)
        return 0;

    if (item_index_1based < 1u || item_index_1based > menu->max_items)
        return 0;

    if (menu->item_names == 0)
        return 0;

    return (PGM_P)pgm_read_ptr(&menu->item_names[item_index_1based - 1u]);
}

PGM_P menu_get_soft_key(const Menu_t *menu, uint8_t key_pos_1based)
{
    if (menu == 0 || key_pos_1based < 1u || key_pos_1based > 3u)
        return 0;
    if (menu->soft_key_bar == 0)
        return 0;
    return (PGM_P)pgm_read_ptr(&menu->soft_key_bar[key_pos_1based - 1u]);
}

menu_softkey_cb_t menu_get_soft_key_action(const Menu_t *menu, uint8_t key_pos_1based)
{
    if (menu == 0 || key_pos_1based < 1u || key_pos_1based > 3u)
        return 0;
    if (menu->soft_key_actions == 0)
        return 0;
    return (menu_softkey_cb_t)pgm_read_ptr(&menu->soft_key_actions[key_pos_1based - 1u]);
}

uint8_t menu_get_max_items(const Menu_t *menu)
{
    if (menu == 0)
        return 0u;
    return menu->max_items;
}

uint8_t menu_get_selected(const Menu_t *menu)
{
    if (menu == 0)
        return 0u;
    return menu->selected;
}

uint8_t menu_get_selected_item(const Menu_t *menu)
{
    if (menu == 0 || menu->selected == 0u)
        return 0u;
    if (menu->selected <= menu->max_items)
        return menu->selected;
    return 0u;
}

uint8_t menu_get_selected_softkey_pos(const Menu_t *menu)
{
    if (menu == 0 || menu->selected == 0u)
        return 0u;
    if (menu->selected <= menu->max_items)
        return 0u;
    return menu_softkey_pos_from_index(menu, (uint8_t)(menu->selected - menu->max_items));
}

uint8_t menu_get_total_selectable(const Menu_t *menu)
{
    if (menu == 0)
        return 0u;
    return (uint8_t)(menu->max_items + menu_softkey_count(menu));
}
