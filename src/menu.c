#include "menu.h"
#include <string.h>
#include <avr/pgmspace.h>

/**
 * Internal structure definition (hidden from users)
 * PGM_P is a typedef for const char* in PROGMEM space (on AVR) and not in RAM
 * because we want to save RAM on AVR targets.
 */
struct Menu
{
    uint8_t selected;  /* 1..max_items */
    uint8_t max_items; /* includes Back */

    PGM_P menu_name;
    PGM_P const *item_names; /* PROGMEM array of item strings (no Back) */

    uint8_t prev_index;
    uint8_t sub_index[MENU_MAX_ITEMS - 1u]; /* submenu indices for items excluding "Back" */
};

/* Global pointers */
Menu_t *g_root_menu = 0;
Menu_t *g_current_menu = 0;

static Menu_t s_menu_pool[MENU_MAX_MENUS];
static uint8_t s_menu_pool_used = 0u;

#define MENU_INDEX_NONE 0xFFu

static uint8_t menu_index_from_ptr(const Menu_t *m)
{
    if (m == 0)
    {
        return MENU_INDEX_NONE;
    }
    return (uint8_t)(m - &s_menu_pool[0]);
}

static Menu_t *menu_from_index(uint8_t idx)
{
    if (idx == MENU_INDEX_NONE)
    {
        return 0;
    }
    return &s_menu_pool[idx];
}

Menu_t *menu_create(void)
{
    if (s_menu_pool_used >= MENU_MAX_MENUS)
    {
        return 0;
    }

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
    {
        return 0u;
    }

    if (value < 1u)
    {
        return max_items;
    }
    if (value > max_items)
    {
        return 1u;
    }
    return value;
}

/* ---- Public API ---- */

menu_status_t menu_init(Menu_t *menu,
                        uint8_t items_without_back,
                        PGM_P const *item_names)
{
    if (menu == 0)
    {
        return MENU_ERROR;
    }

    /* We reserve the last slot for "Back" */
    if (items_without_back > (MENU_MAX_ITEMS - 1u))
    {
        return MENU_ERROR;
    }
    if ((items_without_back > 0u) && (item_names == 0))
    {
        return MENU_ERROR;
    }

    /* Clear structure fields that matter */
    menu->prev_index = MENU_INDEX_NONE;
    for (uint8_t i = 0u; i < (MENU_MAX_ITEMS - 1u); i++)
    {
        menu->sub_index[i] = MENU_INDEX_NONE;
    }

    /* Default name */
    // menu->menu_name = "Menu";
    menu->menu_name = PSTR("Menu"); // PSTR macro places string in PROGMEM

    /* Keep reference to PROGMEM item list (Back handled separately) */
    menu->item_names = item_names;

    /* Total items includes Back */
    menu->max_items = (uint8_t)(items_without_back + 1u);

    /* Start selection at first item */
    menu->selected = 1u;

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

menu_status_t menu_set_submenu(Menu_t *parent, uint8_t item_index_1based, Menu_t *submenu)
{
    if (parent == 0 || submenu == 0)
    {
        return MENU_ERROR;
    }

    if (parent->max_items < 2u)
    {
        /* Must have at least one normal item + Back */
        return MENU_ERROR;
    }

    /* Last item is Back -> cannot have submenu */
    if (item_index_1based < 1u || item_index_1based > (uint8_t)(parent->max_items - 1u))
    {
        return MENU_ERROR;
    }

    uint8_t submenu_idx = menu_index_from_ptr(submenu);
    uint8_t parent_idx = menu_index_from_ptr(parent);
    if ((submenu_idx == MENU_INDEX_NONE) || (parent_idx == MENU_INDEX_NONE))
    {
        return MENU_ERROR;
    }

    parent->sub_index[item_index_1based - 1u] = submenu_idx;
    submenu->prev_index = parent_idx;
    return MENU_OK;
}

uint8_t menu_select(Menu_t *menu, uint8_t item_index_1based)
{
    if (menu == 0)
    {
        return 0u;
    }

    if (menu->max_items == 0u)
    {
        /* Not initialized */
        menu->selected = 0u;
        return 0u;
    }

    menu->selected = clamp_wrap_1based(item_index_1based, menu->max_items);
    return menu->selected;
}

uint8_t menu_next(Menu_t *menu)
{
    if (menu == 0)
    {
        return 0u;
    }
    return menu_select(menu, (uint8_t)(menu->selected + 1u));
}

uint8_t menu_prev(Menu_t *menu)
{
    if (menu == 0)
    {
        return 0u;
    }
    /* If selected is 1, selected-1 underflows; clamp_wrap_1based handles it */
    return menu_select(menu, (uint8_t)(menu->selected - 1u));
}

menu_action_t menu_enter_selected(Menu_t *menu)
{
    if (menu == 0 || menu->max_items == 0u || menu->selected == 0u)
    {
        return MENU_ACTION_NONE;
    }

    /* Back is always the last item */
    if (menu->selected == menu->max_items)
    {
        if (menu->prev_index != MENU_INDEX_NONE)
        {
            g_current_menu = menu_from_index(menu->prev_index);
            return MENU_ACTION_BACK;
        }
        return MENU_ACTION_NONE; /* Already at root */
    }

    /* Normal item: check submenu link */
    Menu_t *next = menu_from_index(menu->sub_index[menu->selected - 1u]);
    if (next != 0)
    {
        g_current_menu = next;
        return MENU_ACTION_ENTER;
    }

    return MENU_ACTION_NONE;
}

PGM_P menu_get_name(const Menu_t *menu)
{
    if (menu == 0)
    {
        return 0;
    }
    return menu->menu_name;
}

PGM_P menu_get_item_name(const Menu_t *menu, uint8_t item_index_1based)
{
    if (menu == 0 || menu->max_items == 0u)
    {
        return 0;
    }

    if (item_index_1based < 1u || item_index_1based > menu->max_items)
    {
        return 0;
    }

    if (item_index_1based == menu->max_items)
    {
        static const char s_back[] PROGMEM = "Back";
        return s_back;
    }

    if (menu->item_names == 0)
    {
        return 0;
    }

    return (PGM_P)pgm_read_ptr(&menu->item_names[item_index_1based - 1u]);
}

uint8_t menu_get_max_items(const Menu_t *menu)
{
    if (menu == 0)
    {
        return 0u;
    }
    return menu->max_items;
}

uint8_t menu_get_selected(const Menu_t *menu)
{
    if (menu == 0)
    {
        return 0u;
    }
    return menu->selected;
}
