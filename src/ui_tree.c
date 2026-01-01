/* ui_tree.c */
#include "ui_tree.h"

/* ====== Menu pointers (screens) ====== */
static Menu_t *m_main;

static Menu_t *m_batt_simple_list; // Battery simple test (list of modes)
static Menu_t *m_run_cdc;          // Run: Chg-Disch-Chg
static Menu_t *m_opt_cdc;          // Options: Chg-Disch-Chg-Options
static Menu_t *m_run_disch;        // Run: Discharging
static Menu_t *m_run_chg;          // Run: Charging

static Menu_t *m_run_cycles;         // Battery cycles test (RUN)
static Menu_t *m_opt_cycles;         // Options: Battery test-Options
static Menu_t *m_modal_stop_confirm; // Modal: Stop confirm

static Menu_t *m_run_resist; // Resist test (RUN)

static Menu_t *m_logs_stub;      // Logs stub
static Menu_t *m_something_stub; // Something else stub

/* Dummy array for menus with 0 items (only Back will exist) */
static const char *const s_empty[] = {0};

/* ====== Leaf actions (connect UI -> test engine here) ====== */

static void action_stop_cycles(void)
{
    /* TODO: call your engine stop here */
    /* Example: test_engine_stop_cycles(); */
}

static void action_toggle_resist_mode(void)
{
    /* TODO: toggle AC/DC mode */
}

static void action_toggle_resist_start_stop(void)
{
    /* TODO: start/stop resistance measurement */
}

static void action_stop_simple_run(void)
{
    /* TODO: stop current simple test run */
}

static void action_start_charging(void)
{
    /* TODO: start charging sequence (when State=Finished) */
}

/**
 * @brief when OK is pressed and the selected item has no submenu and is not Back
 * @param menu Current menu
 * @param selected_1based Selected item index (1-based)
 * it is called 1based because menu_get_selected() returns 1based, meaning the first item is index 1, not 0
 * 0 is invalid
 * @return void
 */
static void ui_handle_leaf(Menu_t *menu, uint8_t selected_1based)
{
    /* ===== Battery simple: RUN screens ===== */
    if (menu == m_run_cdc)
    {
        /* Items: 1=Options (submenu), 2=STOP (leaf), 3=Back */
        if (selected_1based == 2u)
        {
            action_stop_simple_run();
        }
        return;
    }

    if (menu == m_run_disch)
    {
        /* Items: 1=STOP (leaf), 2=Back */
        if (selected_1based == 1u)
        {
            action_stop_simple_run();
        }
        return;
    }

    if (menu == m_run_chg)
    {
        /* Items: 1=Start (leaf), 2=Back */
        if (selected_1based == 1u)
        {
            action_start_charging();
        }
        return;
    }

    /* ===== Cycles test: modal confirm ===== */
    if (menu == m_modal_stop_confirm)
    {
        /* Items: 1=Sure, 2=Cancel, 3=Back */
        if (selected_1based == 1u)
        {
            action_stop_cycles();
            /* Go back (same effect as selecting Back) */
            menu_select(menu, menu_get_max_items(menu));
            (void)menu_enter_selected(menu);
        }
        else
        {
            /* Cancel or Back -> return to previous screen */
            menu_select(menu, menu_get_max_items(menu));
            (void)menu_enter_selected(menu);
        }
        return;
    }

    /* ===== Resist test: actions ===== */
    if (menu == m_run_resist)
    {
        /* Items: 1=AC/DC, 2=START/STOP, 3=Back */
        if (selected_1based == 1u)
        {
            action_toggle_resist_mode();
        }
        else if (selected_1based == 2u)
        {
            action_toggle_resist_start_stop();
        }
        return;
    }

    /* Stubs: no leaf actions */
}

/* ====== Public API ====== */

/**
 * Build all screens/menus and link the tree
 */
void ui_tree_init(void)
{
    /* ---------- MAIN MENU ---------- */
    {
        // static const char *const items[] = {
        //     "Battery simple test",
        //     "Battery cycles test",
        //     "Res. test (DC/AC)",
        //     "View logs files",
        //     "Something else"};

        // m_main = menu_create();
        // (void)menu_init(m_main, 5u, items);
        // menu_set_name(m_main, "Main menu");
        static const char STR_MAIN0[] PROGMEM = "Btr simple test";
        static const char STR_MAIN1[] PROGMEM = "Btr cycles test";
        static const char STR_MAIN2[] PROGMEM = "Res. test (DC/AC)";
        static const char STR_MAIN3[] PROGMEM = "View logs files";
        static const char STR_MAIN4[] PROGMEM = "Something else";

        static PGM_P const MAIN_ITEMS[] = {
            STR_MAIN0, STR_MAIN1, STR_MAIN2, STR_MAIN3, STR_MAIN4};

        m_main = menu_create();
        (void)menu_init(m_main, 5u, MAIN_ITEMS);
        menu_set_name(m_main, PSTR("Main menu"));
    }

    /* ---------- Battery simple test (LIST) ---------- */
    {
        // static const char *const items[] = {
        //     "Chg-Disch-Chg",
        //     "Discharging",
        //     "Charging"};

        // m_batt_simple_list = menu_create();
        // (void)menu_init(m_batt_simple_list, 3u, items);
        // menu_set_name(m_batt_simple_list, "Battery simple test");
        static const char STR_BST0[] PROGMEM = "Chg-Disch-Chg";
        static const char STR_BST1[] PROGMEM = "Discharging";
        static const char STR_BST2[] PROGMEM = "Charging";
        static PGM_P const BST_ITEMS[] = {
            STR_BST0, STR_BST1, STR_BST2};
        m_batt_simple_list = menu_create();
        (void)menu_init(m_batt_simple_list, 3u, BST_ITEMS);
        menu_set_name(m_batt_simple_list, PSTR("Btr simple test"));        
    }

    /* RUN: Chg-Disch-Chg */
    {
        // static const char *const items[] = {
        //     "Options",
        //     "STOP"};

        // m_run_cdc = menu_create();
        // (void)menu_init(m_run_cdc, 2u, items);
        // menu_set_name(m_run_cdc, "Chg-Disch-Chg");
        static const char STR_RCDC0[] PROGMEM = "Options";
        static const char STR_RCDC1[] PROGMEM = "STOP";
        static PGM_P const RCDC_ITEMS[] = {
            STR_RCDC0, STR_RCDC1};
        m_run_cdc = menu_create();
        (void)menu_init(m_run_cdc, 2u, RCDC_ITEMS);
        menu_set_name(m_run_cdc, PSTR("Chg-Disch-Chg"));
    }

    /* OPTIONS: Chg-Disch-Chg-Options (Back only) */
    {
        m_opt_cdc = menu_create();
        (void)menu_init(m_opt_cdc, 0u, s_empty);
        // menu_set_name(m_opt_cdc, "Chg-Disch-Chg-Options");
        menu_set_name(m_opt_cdc, PSTR("Chg-Disch-Chg-Opts"));
    }

    /* RUN: Discharging */
    {
        // static const char *const items[] = {
        //     "STOP"};

        // m_run_disch = menu_create();
        // (void)menu_init(m_run_disch, 1u, items);
        // menu_set_name(m_run_disch, "Discharging");
        static const char STR_RD0[] PROGMEM = "STOP";
        static PGM_P const RD_ITEMS[] = {
            STR_RD0};
        m_run_disch = menu_create();
        (void)menu_init(m_run_disch, 1u, RD_ITEMS);
        menu_set_name(m_run_disch, PSTR("Discharging"));
    }

    /* RUN: Charging (Finished -> Start) */
    {
        // static const char *const items[] = {
        //     "Start"};

        // m_run_chg = menu_create();
        // (void)menu_init(m_run_chg, 1u, items);
        // menu_set_name(m_run_chg, "Charging");
        static const char STR_RC0[] PROGMEM = "Start";
        static PGM_P const RC_ITEMS[] = {
            STR_RC0};
        m_run_chg = menu_create();
        (void)menu_init(m_run_chg, 1u, RC_ITEMS);
        menu_set_name(m_run_chg, PSTR("Charging"));
    }

    /* ---------- Battery cycles test (RUN) ---------- */
    {
        // static const char *const items[] = {
        //     "Options",
        //     "STOP"};

        // m_run_cycles = menu_create();
        // (void)menu_init(m_run_cycles, 2u, items);
        // menu_set_name(m_run_cycles, "Battery cycles test");
        static const char STR_RCYC0[] PROGMEM = "Options";
        static const char STR_RCYC1[] PROGMEM = "STOP";
        static PGM_P const RCYC_ITEMS[] = {
            STR_RCYC0, STR_RCYC1};
        m_run_cycles = menu_create();
        (void)menu_init(m_run_cycles, 2u, RCYC_ITEMS);
        menu_set_name(m_run_cycles, PSTR("Btr cycles test"));
    }

    /* OPTIONS: Battery test-Options (Back only) */
    {
        m_opt_cycles = menu_create();
        (void)menu_init(m_opt_cycles, 0u, s_empty);
        // menu_set_name(m_opt_cycles, "Battery test-Options");
        menu_set_name(m_opt_cycles, PSTR("Btr test-Opts"));
    }

    /* MODAL: Stop confirm */
    {
        // static const char *const items[] = {
        //     "Sure",
        //     "Cancel"};

        // m_modal_stop_confirm = menu_create();
        // (void)menu_init(m_modal_stop_confirm, 2u, items);
        // menu_set_name(m_modal_stop_confirm, "Stop?");
        static const char STR_MSC0[] PROGMEM = "Sure";
        static const char STR_MSC1[] PROGMEM = "Cancel";
        static PGM_P const MSC_ITEMS[] = {
            STR_MSC0, STR_MSC1};
        m_modal_stop_confirm = menu_create();
        (void)menu_init(m_modal_stop_confirm, 2u, MSC_ITEMS);
        menu_set_name(m_modal_stop_confirm, PSTR("Stop?"));
    }

    /* ---------- Resist test (RUN) ---------- */
    {
        // static const char *const items[] = {
        //     "AC/DC",
        //     "START/STOP"};

        // m_run_resist = menu_create();
        // (void)menu_init(m_run_resist, 2u, items);
        // menu_set_name(m_run_resist, "Resist. test (DC/AC)");
        static const char STR_RRT0[] PROGMEM = "AC/DC";
        static const char STR_RRT1[] PROGMEM = "START/STOP";
        static PGM_P const RRT_ITEMS[] = {
            STR_RRT0, STR_RRT1};
        m_run_resist = menu_create();
        (void)menu_init(m_run_resist, 2u, RRT_ITEMS);
        menu_set_name(m_run_resist, PSTR("Resist. test (DC/AC)"));
    }

    /* ---------- Logs stub ---------- */
    {
        m_logs_stub = menu_create();
        (void)menu_init(m_logs_stub, 0u, s_empty);
        // menu_set_name(m_logs_stub, "Logs");
        menu_set_name(m_logs_stub, PSTR("Logs"));
    }

    /* ---------- Something else stub ---------- */
    {
        m_something_stub = menu_create();
        (void)menu_init(m_something_stub, 0u, s_empty);
        // menu_set_name(m_something_stub, "Something else");
        menu_set_name(m_something_stub, PSTR("Something else"));
    }

    /* ====== Link the tree (submenus) ====== */
    (void)menu_set_submenu(m_main, 1u, m_batt_simple_list);
    (void)menu_set_submenu(m_main, 2u, m_run_cycles);
    (void)menu_set_submenu(m_main, 3u, m_run_resist);
    (void)menu_set_submenu(m_main, 4u, m_logs_stub);
    (void)menu_set_submenu(m_main, 5u, m_something_stub);

    (void)menu_set_submenu(m_batt_simple_list, 1u, m_run_cdc);
    (void)menu_set_submenu(m_batt_simple_list, 2u, m_run_disch);
    (void)menu_set_submenu(m_batt_simple_list, 3u, m_run_chg);

    (void)menu_set_submenu(m_run_cdc, 1u, m_opt_cdc);

    (void)menu_set_submenu(m_run_cycles, 1u, m_opt_cycles);
    (void)menu_set_submenu(m_run_cycles, 2u, m_modal_stop_confirm);

    /* After linking, root/current should be main */
    g_root_menu = m_main;
    g_current_menu = m_main;
}

/**
 * Handle button events.
 * @param btn Button pressed
 * it is called when a button is pressed
 * and the parameter btn indicates which button was pressed
 * UI_BTN_PLUS: move selection up
 * UI_BTN_MINUS: move selection down
 * UI_BTN_OK: select the current item
 * it is assumed that the button debouncing is handled elsewhere
 * and is better to call in interrupt context
 * @return void
 */
void ui_on_button(ui_button_t btn)
{
    Menu_t *m = g_current_menu;
    if (m == 0)
    {
        return;
    }

    if (btn == UI_BTN_PLUS)
    {
        (void)menu_next(m);
        return;
    }

    if (btn == UI_BTN_MINUS)
    {
        (void)menu_prev(m);
        return;
    }

    /* OK pressed */
    {
        menu_action_t a = menu_enter_selected(m);

        /* If it was ENTER/BACK, navigation already happened */
        if (a != MENU_ACTION_NONE)
        {
            return;
        }

        /* Otherwise: leaf action (STOP, Start, AC/DC, Sure/Cancel, etc.) */
        ui_handle_leaf(m, menu_get_selected(m));
    }
}
/**
 * Get currently selected item index (1-based).
 * Returns 0 if menu is NULL or no item is selected.
 */
