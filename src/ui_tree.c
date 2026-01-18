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

static void action_soft_back(void)
{
    (void)menu_back(g_current_menu);
}

static void action_open_opt_cdc(void)
{
    g_current_menu = m_opt_cdc;
}

static void action_open_opt_cycles(void)
{
    g_current_menu = m_opt_cycles;
}

static void action_open_stop_confirm(void)
{
    g_current_menu = m_modal_stop_confirm;
}

/**
 * @brief when OK is pressed and the selected item has no submenu
 * @param menu Current menu
 * @param selected_1based Selected item index (1-based)
 * it is called 1based because menu_get_selected() returns 1based (list or soft-key selection)
 * 0 is invalid
 * @return void
 */
static void ui_handle_leaf(Menu_t *menu, uint8_t selected_1based)
{
    /* ===== Battery simple: RUN screens ===== */
    if (menu == m_run_cdc)
    {
        /* Items: 1=Options (submenu), 2=STOP (leaf) */
        if (selected_1based == 2u)
        {
            action_stop_simple_run();
        }
        return;
    }

    if (menu == m_run_disch)
    {
        /* Items: 1=STOP (leaf) */
        if (selected_1based == 1u)
        {
            action_stop_simple_run();
        }
        return;
    }

    if (menu == m_run_chg)
    {
        /* Items: 1=Start (leaf) */
        if (selected_1based == 1u)
        {
            action_start_charging();
        }
        return;
    }

    /* ===== Cycles test: modal confirm ===== */
    if (menu == m_modal_stop_confirm)
    {
        /* Items: 1=Sure, 2=Cancel */
        if (selected_1based == 1u)
        {
            action_stop_cycles();
            (void)menu_back(menu);
        }
        else
        {
            /* Cancel -> return to previous screen */
            (void)menu_back(menu);
        }
        return;
    }

    /* ===== Resist test: actions ===== */
    if (menu == m_run_resist)
    {
        /* Items: 1=AC/DC, 2=START/STOP */
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
    static const char SK_BACK[] PROGMEM = "Back";
    static const char SK_OPTIONS[] PROGMEM = "Options";
    static const char SK_STOP[] PROGMEM = "STOP";
    static const char SK_START[] PROGMEM = "Start";
    static const char SK_ACDC[] PROGMEM = "AC/DC";
    static const char SK_STARTSTOP[] PROGMEM = "START/STOP";
    static const char SK_SURE[] PROGMEM = "Sure";
    static const char SK_CANCEL[] PROGMEM = "Cancel";

    static PGM_P const SK_BACK_ONLY[] PROGMEM = {SK_BACK, 0, 0};
    static PGM_P const SK_BACK_OPTIONS_STOP[] PROGMEM = {SK_BACK, SK_OPTIONS, SK_STOP};
    static PGM_P const SK_BACK_STOP[] PROGMEM = {SK_BACK, 0, SK_STOP};
    static PGM_P const SK_BACK_START[] PROGMEM = {SK_BACK, 0, SK_START};
    static PGM_P const SK_BACK_ACDC_STARTSTOP[] PROGMEM = {SK_BACK, SK_ACDC, SK_STARTSTOP};
    static PGM_P const SK_BACK_CANCEL_SURE[] PROGMEM = {SK_BACK, SK_CANCEL, SK_SURE};

    static menu_softkey_cb_t const SK_BACK_ONLY_ACT[] PROGMEM = {action_soft_back, 0, 0};
    static menu_softkey_cb_t const SK_BACK_OPTIONS_STOP_ACT[] PROGMEM = {action_soft_back, action_open_opt_cdc, action_stop_simple_run};
    static menu_softkey_cb_t const SK_BACK_STOP_ACT[] PROGMEM = {action_soft_back, 0, action_stop_simple_run};
    static menu_softkey_cb_t const SK_BACK_START_ACT[] PROGMEM = {action_soft_back, 0, action_start_charging};
    static menu_softkey_cb_t const SK_BACK_ACDC_STARTSTOP_ACT[] PROGMEM = {action_soft_back, action_toggle_resist_mode, action_toggle_resist_start_stop};
    static menu_softkey_cb_t const SK_BACK_CANCEL_SURE_ACT[] PROGMEM = {action_soft_back, action_soft_back, action_stop_cycles};

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
        static const char STR_MAIN0[] PROGMEM = "1.Battery simple test";
        static const char STR_MAIN1[] PROGMEM = "2.Battery cycles test";
        static const char STR_MAIN2[] PROGMEM = "3.Res. test (DC/AC)";
        static const char STR_MAIN3[] PROGMEM = "4.View logs files";

        static PGM_P const MAIN_ITEMS[] PROGMEM = {
            STR_MAIN0, STR_MAIN1, STR_MAIN2, STR_MAIN3};

        m_main = menu_create();
        (void)menu_init(m_main, sizeof(MAIN_ITEMS) / sizeof(MAIN_ITEMS[0]), MAIN_ITEMS);
        menu_set_name(m_main, PSTR("   ---Main menu---"));
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
        static const char STR_BST0[] PROGMEM = "1.Chg-Disch-Chg";
        static const char STR_BST1[] PROGMEM = "2.Discharging";
        static const char STR_BST2[] PROGMEM = "3.Charging";
        static PGM_P const BST_ITEMS[] PROGMEM = {
            STR_BST0, STR_BST1, STR_BST2};
        m_batt_simple_list = menu_create();
        (void)menu_init(m_batt_simple_list, 3u, BST_ITEMS);
        menu_set_name(m_batt_simple_list, PSTR(" --Btr simple test--"));        
        menu_set_soft_keys(m_batt_simple_list, SK_BACK_ONLY, SK_BACK_ONLY_ACT);
    }

    /* RUN: Chg-Disch-Chg */
    {
        // static const char *const items[] = {
        //     "Options",
        //     "STOP"};

        // m_run_cdc = menu_create();
        // (void)menu_init(m_run_cdc, 2u, items);
        // menu_set_name(m_run_cdc, "Chg-Disch-Chg");
        static const char STR_RCDC0[] PROGMEM = " ";
        static const char STR_RCDC1[] PROGMEM = " ";
        static PGM_P const RCDC_ITEMS[] PROGMEM = {
            STR_RCDC0, STR_RCDC1};
        m_run_cdc = menu_create();
        (void)menu_init(m_run_cdc, 2u, RCDC_ITEMS);
        menu_set_name(m_run_cdc, PSTR("   -Chg-Disch-Chg-"));
        menu_set_soft_keys(m_run_cdc, SK_BACK_OPTIONS_STOP, SK_BACK_OPTIONS_STOP_ACT);
    }

    /* OPTIONS: Chg-Disch-Chg-Options (Back only) */
    {
        m_opt_cdc = menu_create();
        (void)menu_init(m_opt_cdc, 0u, 0);
        // menu_set_name(m_opt_cdc, "Chg-Disch-Chg-Options");
        menu_set_name(m_opt_cdc, PSTR(" -Chg-Disch-Chg-Opts-"));
        menu_set_soft_keys(m_opt_cdc, SK_BACK_ONLY, SK_BACK_ONLY_ACT);
    }

    /* RUN: Discharging */
    {
        // static const char *const items[] = {
        //     "STOP"};

        // m_run_disch = menu_create();
        // (void)menu_init(m_run_disch, 1u, items);
        // menu_set_name(m_run_disch, "Discharging");
        static const char STR_RD0[] PROGMEM = " ";
        static PGM_P const RD_ITEMS[] PROGMEM = {
            STR_RD0};
        m_run_disch = menu_create();
        (void)menu_init(m_run_disch, 1u, RD_ITEMS);
        menu_set_name(m_run_disch, PSTR("    -Discharging-"));
        menu_set_soft_keys(m_run_disch, SK_BACK_STOP, SK_BACK_STOP_ACT);
    }

    /* RUN: Charging (Finished -> Start) */
    {
        // static const char *const items[] = {
        //     "Start"};

        // m_run_chg = menu_create();
        // (void)menu_init(m_run_chg, 1u, items);
        // menu_set_name(m_run_chg, "Charging");
        static const char STR_RC0[] PROGMEM = " ";
        static PGM_P const RC_ITEMS[] PROGMEM = {
            STR_RC0};
        m_run_chg = menu_create();
        (void)menu_init(m_run_chg, 1u, RC_ITEMS);
        menu_set_name(m_run_chg, PSTR("     -Charging-"));
        menu_set_soft_keys(m_run_chg, SK_BACK_START, SK_BACK_START_ACT);
    }

    /* ---------- Battery cycles test (RUN) ---------- */
    {
        // static const char *const items[] = {
        //     "Options",
        //     "STOP"};

        // m_run_cycles = menu_create();
        // (void)menu_init(m_run_cycles, 2u, items);
        // menu_set_name(m_run_cycles, "Battery cycles test");
        static const char STR_RCYC0[] PROGMEM = " ";
        static const char STR_RCYC1[] PROGMEM = " ";
        static PGM_P const RCYC_ITEMS[] PROGMEM = {
            STR_RCYC0, STR_RCYC1};
        m_run_cycles = menu_create();
        (void)menu_init(m_run_cycles, 2u, RCYC_ITEMS);
        menu_set_name(m_run_cycles, PSTR(" --Btr cycles test--"));
        static menu_softkey_cb_t const SK_BACK_OPTIONS_STOP_ACT_CYC[] PROGMEM = {action_soft_back, action_open_opt_cycles, action_open_stop_confirm};
        menu_set_soft_keys(m_run_cycles, SK_BACK_OPTIONS_STOP, SK_BACK_OPTIONS_STOP_ACT_CYC);
    }

    /* OPTIONS: Battery test-Options (Back only) */
    {
        m_opt_cycles = menu_create();
        (void)menu_init(m_opt_cycles, 0u, 0);
        // menu_set_name(m_opt_cycles, "Battery test-Options");
        menu_set_name(m_opt_cycles, PSTR("   -Btr test-Opts-"));
        menu_set_soft_keys(m_opt_cycles, SK_BACK_ONLY, SK_BACK_ONLY_ACT);
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
        static PGM_P const MSC_ITEMS[] PROGMEM = {
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
        static PGM_P const RRT_ITEMS[] PROGMEM = {
            STR_RRT0, STR_RRT1};
        m_run_resist = menu_create();
        (void)menu_init(m_run_resist, 2u, RRT_ITEMS);
        menu_set_name(m_run_resist, PSTR(" Resist. test (DC/AC)"));
        menu_set_soft_keys(m_run_resist, SK_BACK_ACDC_STARTSTOP, SK_BACK_ACDC_STARTSTOP_ACT);
    }

    /* ---------- Logs stub ---------- */
    {
        m_logs_stub = menu_create();
        (void)menu_init(m_logs_stub, 0u, 0);
        // menu_set_name(m_logs_stub, "Logs");
        menu_set_name(m_logs_stub, PSTR("       -Logs-"));
        menu_set_soft_keys(m_logs_stub, SK_BACK_ONLY, SK_BACK_ONLY_ACT);
    }

    /* ---------- Something else stub ---------- */
    {
        m_something_stub = menu_create();
        (void)menu_init(m_something_stub, 0u, 0);
        // menu_set_name(m_something_stub, "Something else");
        menu_set_name(m_something_stub, PSTR("  -Something else-"));
        menu_set_soft_keys(m_something_stub, SK_BACK_ONLY, SK_BACK_ONLY_ACT);
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

        /* If it was ENTER/BACK/SOFTKEY, navigation already happened */
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
