#include "menu.h"
#include "menu_internal.h"
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
 * - Lines 3 .. (LAST_LINE-1): Scrollable content (context) area
 *   - list items (submenu entries or leaf_menu items or live parameters)
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
/* Global pointers */
Menu_t *g_root_menu = 0;
Menu_t *g_current_menu = 0;

static PGM_P menu_softkey_label(const Menu_t *menu, uint8_t pos_1based)
{
    if (menu == 0 || menu->soft_keys == 0 || pos_1based < 1u || pos_1based > 3u)
        return 0;
    return (PGM_P)pgm_read_ptr(&menu->soft_keys[pos_1based - 1u].label);
}

static menu_softkey_cb_t menu_softkey_cb(const Menu_t *menu, uint8_t pos_1based)
{
    if (menu == 0 || menu->soft_keys == 0 || pos_1based < 1u || pos_1based > 3u)
        return 0;
    return (menu_softkey_cb_t)pgm_read_ptr(&menu->soft_keys[pos_1based - 1u].cb);
}

static uint8_t menu_softkey_initial_state(const Menu_t *menu, uint8_t pos_1based)
{
    if (menu == 0 || menu->soft_keys == 0 || pos_1based < 1u || pos_1based > 3u)
        return 0u;
    return (uint8_t)pgm_read_byte(&menu->soft_keys[pos_1based - 1u].initial_state);
}

static uint8_t menu_softkey_count(const Menu_t *menu)
{
    if (menu == 0)
        return 0u;

    uint8_t count = 0u;
    for (uint8_t pos = 1u; pos <= 3u; pos++)
    {
        if (menu_softkey_label(menu, pos) != 0)
            count++;
    }
    return count;
}

static uint8_t menu_softkey_pos_from_index(const Menu_t *menu, uint8_t index_1based)
{
    if (menu == 0 || index_1based == 0u)
        return 0u;

    uint8_t count = 0u;
    for (uint8_t pos = 1u; pos <= 3u; pos++)
    {
        if (menu_softkey_label(menu, pos) != 0)
        {
            count++;
            if (count == index_1based)
                return pos;
        }
    }
    return 0u;
}

static uint8_t count_segments(PGM_P label);
static uint8_t copy_segment(PGM_P label, uint8_t seg_index, char *buf, uint8_t buf_size);
static uint8_t menu_leaf_editable_count(const Menu_t *menu);
static uint8_t menu_leaf_row_from_selectable_index(const Menu_t *menu, uint8_t sel_index);
static uint8_t menu_leaf_row_is_editable_internal(const Menu_t *menu, uint8_t row_index_1based);

Menu_t *menu_create(void)
{
    return 0;
}

void menu_init(Menu_t *menu)
{
    if (menu == 0)
        return;

    menu->selected = 0u;
    menu->menu_items_count = 0u;
    menu->leaf_items_count = 0u;
    menu->param_edit_flag = 0u;
    menu->curr_edit_row_idx = 0u;

    menu->menu_name = PSTR("Menu");
    menu->status_left.prefix = 0;
    menu->status_left.value = 0;
    menu->status_right.prefix = 0;
    menu->status_right.value = 0;

    menu->menu_rows = 0;
    menu->leaf_rows = 0;
    menu->leaf_render = 0;

    menu->soft_keys = 0;
    menu->soft_key_state[0] = 0u;
    menu->soft_key_state[1] = 0u;
    menu->soft_key_state[2] = 0u;
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

menu_status_t menu_init_list(Menu_t *menu,
                             uint8_t items_without_back,
                             PGM_P const *item_names)
{
    if (menu == 0)
        return MENU_ERROR;

    if (items_without_back > MENU_MAX_ITEMS)
        return MENU_ERROR;

    if ((items_without_back > 0u) && (item_names == 0))
        return MENU_ERROR;

    menu_init(menu);
    (void)item_names;
    menu->menu_items_count = items_without_back;

    /* Start selection at first selectable item (if any) */
    menu->selected = (menu_get_total_selectable(menu) > 0u) ? 1u : 0u;

    return MENU_OK;
}

menu_status_t menu_init_leaf(Menu_t *menu,
                             uint8_t row_count,
                             PGM_P const *row_labels,
                             const menu_get_u16_fn_t *getters,
                             const menu_set_u16_fn_t *setters,
                             const uint8_t *editable_flags)
{
    if (menu == 0)
        return MENU_ERROR;

    if (row_count > MENU_MAX_ITEMS)
        return MENU_ERROR;

    if ((row_count > 0u) && (row_labels == 0))
        return MENU_ERROR;

    menu_init(menu);
    (void)row_labels;
    (void)getters;
    (void)setters;
    (void)editable_flags;
    menu->leaf_items_count = row_count;

    menu->selected = (menu_get_total_selectable(menu) > 0u) ? 1u : 0u;

    return MENU_OK;
}

void menu_set_name(Menu_t *menu, PGM_P name)
{
    if (menu)
        menu->menu_name = name;
}

void menu_set_status(Menu_t *menu,
                     PGM_P left_prefix, const char *left_value,
                     PGM_P right_prefix, const char *right_value)
{
    if (menu == 0)
        return;
    menu->status_left.prefix = left_prefix;
    menu->status_left.value = left_value;
    menu->status_right.prefix = right_prefix;
    menu->status_right.value = right_value;
}

menu_status_t menu_set_menu_rows(Menu_t *menu, uint8_t count, const menu_context_t *rows)
{
    if (menu == 0)
        return MENU_ERROR;
    if (count > MENU_MAX_ITEMS)
        return MENU_ERROR;
    if ((count > 0u) && (rows == 0))
        return MENU_ERROR;

    menu->menu_rows = rows;
    menu->menu_items_count = count;
    for (uint8_t i = 0u; i < count; i++)
    {
        Menu_t *child = (Menu_t *)pgm_read_ptr(&rows[i].submenu);
        if (child != 0)
        {
            child->prev_menu = menu;
        }
    }

    if (menu->selected == 0u && menu_get_total_selectable(menu) > 0u)
        menu->selected = 1u;

    return MENU_OK;
}

menu_status_t menu_set_leaf_rows(Menu_t *menu, uint8_t count, const leaf_context_t *rows)
{
    if (menu == 0)
        return MENU_ERROR;
    if (count > MENU_MAX_ITEMS)
        return MENU_ERROR;
    if ((count > 0u) && (rows == 0))
        return MENU_ERROR;

    menu->leaf_rows = rows;
    menu->leaf_items_count = count;
    menu->param_edit_flag = 0u;
    menu->curr_edit_row_idx = 0u;

    if (menu->selected == 0u && menu_get_total_selectable(menu) > 0u)
        menu->selected = 1u;

    return MENU_OK;
}

menu_status_t menu_set_soft_keys(Menu_t *menu, const soft_key_t *keys)
{
    if (menu == 0)
        return MENU_ERROR;
    menu->soft_keys = keys;
    for (uint8_t pos = 1u; pos <= 3u; pos++)
    {
        uint8_t idx = (uint8_t)(pos - 1u);
        if (keys)
            menu->soft_key_state[idx] = menu_softkey_initial_state(menu, pos);
        else
            menu->soft_key_state[idx] = 0u;
    }
    if (menu->selected == 0u && menu_get_total_selectable(menu) > 0u)
        menu->selected = 1u;
    return MENU_OK;
}

menu_status_t menu_set_list(Menu_t *menu,
                            uint8_t item_count,
                            PGM_P const *item_labels)
{
    if (menu == 0)
        return MENU_ERROR;
    (void)item_labels;
    if (item_count == 0u)
    {
        menu->menu_rows = 0;
        menu->menu_items_count = 0u;
        return MENU_OK;
    }
    return MENU_ERROR;

    if (menu->selected == 0u && menu_get_total_selectable(menu) > 0u)
        menu->selected = 1u;

    return MENU_OK;
}

menu_status_t menu_set_leaf(Menu_t *menu,
                            uint8_t row_count,
                            PGM_P const *row_labels,
                            const menu_get_u16_fn_t *getters,
                            const menu_set_u16_fn_t *setters,
                            const uint8_t *editable_flags)
{
    if (menu == 0)
        return MENU_ERROR;
    (void)row_labels;
    (void)getters;
    (void)setters;
    (void)editable_flags;
    if (row_count == 0u)
    {
        menu->leaf_rows = 0;
        menu->leaf_items_count = 0u;
        return MENU_OK;
    }
    return MENU_ERROR;

    if (menu->selected == 0u && menu_get_total_selectable(menu) > 0u)
        menu->selected = 1u;

    return MENU_OK;
}

void menu_set_leaf_render(Menu_t *menu, menu_leaf_render_fn_t render_fn)
{
    if (menu == 0)
        return;
    menu->leaf_render = render_fn;
}

menu_leaf_render_fn_t menu_get_leaf_render(const Menu_t *menu)
{
    if (menu == 0)
        return 0;
    return menu->leaf_render;
}

menu_status_t menu_set_soft_key(Menu_t *menu,
                                uint8_t key_pos_1based,
                                PGM_P key_label,
                                menu_softkey_cb_t callback_on_ok,
                                uint8_t initial_state)
{
    (void)menu;
    (void)key_pos_1based;
    (void)key_label;
    (void)callback_on_ok;
    (void)initial_state;
    return MENU_ERROR;
}

menu_status_t menu_set_submenu(Menu_t *parent, uint8_t item_index_1based, Menu_t *submenu)
{
    (void)parent;
    (void)item_index_1based;
    (void)submenu;
    return MENU_ERROR;
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

    if (menu->selected <= menu->menu_items_count)
    {
        /* Normal item: check submenu link */
        Menu_t *next = 0;
        if (menu->menu_rows != 0)
        {
            next = (Menu_t *)pgm_read_ptr(&menu->menu_rows[menu->selected - 1u].submenu);
        }
        if (next != 0)
        {
            g_current_menu = next;
            return MENU_ACTION_ENTER;
        }
        return MENU_ACTION_NONE;
    }

    uint8_t editable_count = menu_leaf_editable_count(menu);
    if (menu->selected <= (uint8_t)(menu->menu_items_count + editable_count))
    {
        return MENU_ACTION_NONE;
    }

    /* Soft-key action */
    uint8_t soft_index = (uint8_t)(menu->selected - (menu->menu_items_count + editable_count));
    uint8_t pos = menu_softkey_pos_from_index(menu, soft_index);
    if (pos != 0u)
    {
        menu_softkey_cb_t cb = menu_get_soft_key_action(menu, pos);
        if (cb)
            cb();

        PGM_P label = menu_softkey_label(menu, pos);
        uint8_t seg_count = count_segments(label);
        if (seg_count > 1u)
        {
            uint8_t state = menu->soft_key_state[pos - 1u];
            uint8_t current = (state == 0u) ? 1u : (uint8_t)(((state - 1u) % seg_count) + 1u);
            uint8_t next = (uint8_t)((current % seg_count) + 1u);
            menu->soft_key_state[pos - 1u] = next;
        }
        return MENU_ACTION_SOFTKEY;
    }
    return MENU_ACTION_NONE;
}

menu_status_t menu_back(Menu_t *menu)
{
    if (menu == 0)
        return MENU_ERROR;

    if (menu->prev_menu != 0)
    {
        g_current_menu = menu->prev_menu;
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

PGM_P menu_get_status_left_prefix(const Menu_t *menu)
{
    if (menu == 0)
        return 0;
    return menu->status_left.prefix;
}
const char *menu_get_status_left_value(const Menu_t *menu)
{
    if (menu == 0)
        return 0;
    return menu->status_left.value;
}
PGM_P menu_get_status_right_prefix(const Menu_t *menu)
{
    if (menu == 0)
        return 0;
    return menu->status_right.prefix;
}
const char *menu_get_status_right_value(const Menu_t *menu)
{
    if (menu == 0)
        return 0;
    return menu->status_right.value;
}

PGM_P menu_get_list_item_label(const Menu_t *menu, uint8_t item_index_1based)
{
    if (menu == 0 || menu->menu_items_count == 0u)
        return 0;

    if (item_index_1based < 1u || item_index_1based > menu->menu_items_count)
        return 0;

    if (menu->menu_rows == 0)
        return 0;
    return (PGM_P)pgm_read_ptr(&menu->menu_rows[item_index_1based - 1u].label);
}

menu_softkey_cb_t menu_get_soft_key_action(const Menu_t *menu, uint8_t key_pos_1based)
{
    if (menu == 0 || key_pos_1based < 1u || key_pos_1based > 3u)
        return 0;
    return menu_softkey_cb(menu, key_pos_1based);
}

uint8_t menu_get_list_count(const Menu_t *menu)
{
    if (menu == 0)
        return 0u;
    return menu->menu_items_count;
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
    if (menu->selected <= menu->menu_items_count)
        return menu->selected;
    return 0u;
}

uint8_t menu_get_selected_softkey_pos(const Menu_t *menu)
{
    if (menu == 0 || menu->selected == 0u)
        return 0u;
    if (menu->selected <= menu->menu_items_count)
        return 0u;
    if (menu->selected <= (uint8_t)(menu->menu_items_count + menu_leaf_editable_count(menu)))
        return 0u;
    return menu_softkey_pos_from_index(menu,
                                       (uint8_t)(menu->selected - (menu->menu_items_count + menu_leaf_editable_count(menu))));
}

uint8_t menu_get_total_selectable(const Menu_t *menu)
{
    if (menu == 0)
        return 0u;
    return (uint8_t)(menu->menu_items_count + menu_leaf_editable_count(menu) + menu_softkey_count(menu));
}

static uint8_t count_segments(PGM_P label)
{
    uint8_t count = 1u;
    if (label == 0)
        return 0u;
    for (uint16_t i = 0u;; i++)
    {
        char c = (char)pgm_read_byte(label + i);
        if (c == '\0')
            break;
        if (c == '|')
            count++;
    }
    return count;
}

static uint8_t copy_segment(PGM_P label, uint8_t seg_index, char *buf, uint8_t buf_size)
{
    if (buf == 0 || buf_size == 0u)
        return 0u;
    buf[0] = '\0';
    if (label == 0 || seg_index == 0u)
        return 0u;

    uint8_t cur_seg = 1u;
    uint8_t out = 0u;
    for (uint16_t i = 0u;; i++)
    {
        char c = (char)pgm_read_byte(label + i);
        if (c == '\0')
            break;
        if (c == '|')
        {
            cur_seg++;
            continue;
        }
        if (cur_seg == seg_index)
        {
            if ((out + 1u) < buf_size)
            {
                buf[out++] = c;
            }
        }
    }
    buf[out] = '\0';
    return out;
}

uint8_t menu_get_soft_key_text(const Menu_t *menu, uint8_t key_pos_1based, char *buf, uint8_t buf_size)
{
    if (menu == 0 || key_pos_1based < 1u || key_pos_1based > 3u)
        return 0u;

    PGM_P label = menu_softkey_label(menu, key_pos_1based);
    if (label == 0)
        return 0u;

    uint8_t seg_count = count_segments(label);
    if (seg_count <= 1u)
    {
        return copy_segment(label, 1u, buf, buf_size);
    }

    uint8_t seg_index = 1u;
    uint8_t state = menu->soft_key_state[key_pos_1based - 1u];
    if (state == 0u)
    {
        seg_index = 1u;
    }
    else
    {
        uint8_t cur = (uint8_t)(((state - 1u) % seg_count) + 1u);
        seg_index = (uint8_t)((cur % seg_count) + 1u);
    }
    return copy_segment(label, seg_index, buf, buf_size);
}

uint8_t menu_get_selected_leaf_row(const Menu_t *menu)
{
    if (menu == 0 || menu->selected == 0u)
        return 0u;

    uint8_t editable_count = menu_leaf_editable_count(menu);
    if (menu->selected <= menu->menu_items_count)
        return 0u;
    if (menu->selected > (uint8_t)(menu->menu_items_count + editable_count))
        return 0u;

    return menu_leaf_row_from_selectable_index(menu, (uint8_t)(menu->selected - menu->menu_items_count));
}

uint8_t menu_is_leaf(const Menu_t *menu)
{
    if (menu == 0)
        return 0u;
    return (menu->leaf_rows != 0 && menu->leaf_items_count > 0u) ? 1u : 0u;
}

uint8_t menu_is_edit_mode(const Menu_t *menu)
{
    if (menu == 0)
        return 0u;
    return menu->param_edit_flag;
}

uint8_t menu_get_leaf_row_count(const Menu_t *menu)
{
    if (menu == 0 || menu->leaf_rows == 0)
        return 0u;
    return menu->leaf_items_count;
}

PGM_P menu_get_leaf_row_label(const Menu_t *menu, uint8_t row_index_1based)
{
    if (menu == 0 || row_index_1based == 0u || row_index_1based > menu->leaf_items_count)
        return 0;
    if (menu->leaf_rows == 0)
        return 0;
    return (PGM_P)pgm_read_ptr(&menu->leaf_rows[row_index_1based - 1u].label);
}

uint16_t menu_get_leaf_row_value(const Menu_t *menu, uint8_t row_index_1based)
{
    if (menu == 0 || row_index_1based == 0u || row_index_1based > menu->leaf_items_count)
        return 0u;
    if (menu->leaf_rows == 0)
        return 0u;
    menu_get_u16_fn_t get = (menu_get_u16_fn_t)pgm_read_ptr(&menu->leaf_rows[row_index_1based - 1u].get);
    return get ? get() : 0u;
}

uint8_t menu_leaf_row_is_editable(const Menu_t *menu, uint8_t row_index_1based)
{
    return menu_leaf_row_is_editable_internal(menu, row_index_1based);
}

menu_action_t menu_on_ok(Menu_t *menu)
{
    if (menu == 0 || menu->selected == 0u)
        return MENU_ACTION_NONE;

    if (menu->selected <= menu->menu_items_count)
        return menu_enter_selected(menu);

    uint8_t editable_count = menu_leaf_editable_count(menu);
    if (menu->selected <= (uint8_t)(menu->menu_items_count + editable_count))
    {
        uint8_t row = menu_leaf_row_from_selectable_index(menu, (uint8_t)(menu->selected - menu->menu_items_count));
        if (row != 0u && menu_leaf_row_is_editable(menu, row))
        {
            if (menu->param_edit_flag && menu->curr_edit_row_idx == row)
            {
                menu->param_edit_flag = 0u;
                menu->curr_edit_row_idx = 0u;
            }
            else
            {
                menu->param_edit_flag = 1u;
                menu->curr_edit_row_idx = row;
            }
        }
        return MENU_ACTION_NONE;
    }

    uint8_t soft_index = (uint8_t)(menu->selected - (menu->menu_items_count + editable_count));
    uint8_t pos = menu_softkey_pos_from_index(menu, soft_index);
    if (pos != 0u)
    {
        menu_softkey_cb_t cb = menu_get_soft_key_action(menu, pos);
        if (cb)
            cb();

        PGM_P label = menu_softkey_label(menu, pos);
        uint8_t seg_count = count_segments(label);
        if (seg_count > 1u)
        {
            uint8_t state = menu->soft_key_state[pos - 1u];
            uint8_t current = (state == 0u) ? 1u : (uint8_t)(((state - 1u) % seg_count) + 1u);
            uint8_t next = (uint8_t)((current % seg_count) + 1u);
            menu->soft_key_state[pos - 1u] = next;
        }
        return MENU_ACTION_SOFTKEY;
    }

    return MENU_ACTION_NONE;
}

uint8_t menu_on_plus(Menu_t *menu)
{
    if (menu == 0)
        return 0u;
    if (menu->param_edit_flag)
    {
        uint8_t row = menu->curr_edit_row_idx;
        if (row != 0u)
        {
            if (menu->leaf_rows == 0)
                return menu->selected;
            menu_set_u16_fn_t set = (menu_set_u16_fn_t)pgm_read_ptr(&menu->leaf_rows[row - 1u].set);
            if (set)
            {
                uint16_t val = menu_get_leaf_row_value(menu, row);
                if (val < 0xFFFFu)
                    val++;
                set(val);
            }
        }
        return menu->selected;
    }
    return menu_next(menu);
}

uint8_t menu_on_minus(Menu_t *menu)
{
    if (menu == 0)
        return 0u;
    if (menu->param_edit_flag)
    {
        uint8_t row = menu->curr_edit_row_idx;
        if (row != 0u)
        {
            if (menu->leaf_rows == 0)
                return menu->selected;
            menu_set_u16_fn_t set = (menu_set_u16_fn_t)pgm_read_ptr(&menu->leaf_rows[row - 1u].set);
            if (set)
            {
                uint16_t val = menu_get_leaf_row_value(menu, row);
                if (val > 0u)
                    val--;
                set(val);
            }
        }
        return menu->selected;
    }
    return menu_prev(menu);
}

static uint8_t menu_leaf_row_is_editable_internal(const Menu_t *menu, uint8_t row_index_1based)
{
    if (menu == 0 || row_index_1based == 0u || row_index_1based > menu->leaf_items_count)
        return 0u;
    if (menu->leaf_rows == 0)
        return 0u;
    return pgm_read_byte(&menu->leaf_rows[row_index_1based - 1u].editable) ? 1u : 0u;
}

static uint8_t menu_leaf_editable_count(const Menu_t *menu)
{
    if (menu == 0 || menu->leaf_rows == 0 || menu->leaf_items_count == 0u)
        return 0u;
    uint8_t count = 0u;
    for (uint8_t i = 1u; i <= menu->leaf_items_count; i++)
    {
        if (menu_leaf_row_is_editable_internal(menu, i))
            count++;
    }
    return count;
}

static uint8_t menu_leaf_row_from_selectable_index(const Menu_t *menu, uint8_t sel_index)
{
    if (menu == 0 || sel_index == 0u)
        return 0u;
    uint8_t count = 0u;
    for (uint8_t i = 1u; i <= menu->leaf_items_count; i++)
    {
        if (menu_leaf_row_is_editable_internal(menu, i))
        {
            count++;
            if (count == sel_index)
                return i;
        }
    }
    return 0u;
}

