/* ui_tree.c */
#include "ui_tree.h"
#include "menu_internal.h"
#include "ina_values.h"
#include <avr/io.h>

/* ====== Menu pointers (screens) ====== */
static Menu_t m_main_obj;
static Menu_t m_batt_simple_list_obj;
static Menu_t m_run_cdc_obj;
static Menu_t m_opt_cdc_obj;
static Menu_t m_run_disch_obj;
static Menu_t m_run_chg_obj;
static Menu_t m_run_cycles_obj;
static Menu_t m_opt_cycles_obj;
static Menu_t m_modal_stop_confirm_obj;
static Menu_t m_run_resist_obj;
static Menu_t m_logs_stub_obj;
static Menu_t m_something_stub_obj;

static Menu_t *m_main = &m_main_obj;
static Menu_t *m_batt_simple_list = &m_batt_simple_list_obj;
static Menu_t *m_run_cdc = &m_run_cdc_obj;
static Menu_t *m_opt_cdc = &m_opt_cdc_obj;
static Menu_t *m_run_disch = &m_run_disch_obj;
static Menu_t *m_run_chg = &m_run_chg_obj;
static Menu_t *m_run_cycles = &m_run_cycles_obj;
static Menu_t *m_opt_cycles = &m_opt_cycles_obj;
static Menu_t *m_modal_stop_confirm = &m_modal_stop_confirm_obj;
static Menu_t *m_run_resist = &m_run_resist_obj;
static Menu_t *m_logs_stub = &m_logs_stub_obj;
static Menu_t *m_something_stub = &m_something_stub_obj;

static void action_stop_cycles(void);
static void action_toggle_resist_mode(void);
static void action_toggle_resist_start_stop(void);
static void action_stop_simple_run(void);
static void action_toggle_discharge(void);
static void action_toggle_charging(void);
static void action_soft_back(void);
static void action_open_opt_cdc(void);
static void action_open_opt_cycles(void);
static void action_open_stop_confirm(void);

static const char SK_BACK[] PROGMEM = "Back ";
static const char SK_OPTIONS[] PROGMEM = " Options ";
static const char SK_ACDC[] PROGMEM = " AC | DC ";
static const char SK_STARTSTOP[] PROGMEM = " Start| Stop ";

static const soft_key_t SK_BACK_ONLY[] PROGMEM = {
    {SK_BACK, action_soft_back, 0},
    {0, 0, 0},
    {0, 0, 0}};
static const soft_key_t SK_BACK_OPTIONS_STOP[] PROGMEM = {
    {SK_BACK, action_soft_back, 0},
    {SK_OPTIONS, action_open_opt_cdc, 0},
    {SK_STARTSTOP, action_stop_simple_run, 0}};
static const soft_key_t SK_BACK_TOGGLE_DISCH[] PROGMEM = {
    {SK_BACK, action_soft_back, 0},
    {0, 0, 0},
    {SK_STARTSTOP, action_toggle_discharge, 2}};
static const soft_key_t SK_BACK_TOGGLE_CHG[] PROGMEM = {
    {SK_BACK, action_soft_back, 0},
    {0, 0, 0},
    {SK_STARTSTOP, action_toggle_charging, 2}};
static const soft_key_t SK_BACK_ACDC_STARTSTOP[] PROGMEM = {
    {SK_BACK, action_soft_back, 0},
    {SK_ACDC, action_toggle_resist_mode, 1},
    {SK_STARTSTOP, action_toggle_resist_start_stop, 2}};
static const soft_key_t SK_BACK_OPTIONS_STOP_CYC[] PROGMEM = {
    {SK_BACK, action_soft_back, 0},
    {SK_OPTIONS, action_open_opt_cycles, 0},
    {SK_STARTSTOP, action_open_stop_confirm, 0}};

static uint16_t ui_get_ina_voltage_mv(void)
{
    return g_ina_voltage_mv;
}

static uint16_t ui_get_ina_current_ma(void)
{
    int16_t cur = g_ina_current_ma;
    if (cur < 0)
    {
        cur = (int16_t)(-cur);
    }
    return (uint16_t)cur;
}

static const char STR_INA_VOLT[] PROGMEM = "U= %.3fV";
static const char STR_INA_CURR[] PROGMEM = "I= %.3fA";
static const char STR_INA_CHG_CURR[] PROGMEM = "I= %.3fA";
static const char STR_INA_DIS_CURR[] PROGMEM = "I= -%.3fA";
static const char STR_CHG1_CAP[] PROGMEM = "Charge1:  %i mAh";
static const char STR_CHG2_CAP[] PROGMEM = "Charge2:  %i mAh";
static const char STR_DIS_CAP[] PROGMEM = "Dischg:   %i mAh";
static const char STR_CHG_CAP[] PROGMEM = "Capacity: %i mAh";
static const char STR_ELAP_TIME[] PROGMEM = "El.time: %t";
static const char STR_EMPTY[] PROGMEM = "";

uint16_t ui_get_temp_cap(void)
{
    return 2123u + random(0, 10);
}
uint16_t ui_get_temp_elap_s(void)
{
    return 3661u + random(0, 200);
}

static const leaf_context_t INA_CHG_LEAF_ROWS[] PROGMEM = {
    {STR_INA_VOLT, ui_get_ina_voltage_mv, 0, 0},
    {STR_INA_CHG_CURR, ui_get_ina_current_ma, 0, 0},
    {STR_CHG_CAP, ui_get_temp_cap, 0, 0},
    {STR_ELAP_TIME, ui_get_temp_elap_s, 0, 0}};

static const leaf_context_t INA_DIS_LEAF_ROWS[] PROGMEM = {
    {STR_INA_VOLT, ui_get_ina_voltage_mv, 0, 0},
    {STR_INA_DIS_CURR, ui_get_ina_current_ma, 0, 0},
    {STR_CHG_CAP, ui_get_temp_cap, 0, 0},
    {STR_ELAP_TIME, ui_get_temp_elap_s, 0, 0}};

static const leaf_context_t INA_LEAF_ROWS[] PROGMEM = {
    {STR_INA_VOLT, ui_get_ina_voltage_mv, 0, 0},
    {STR_INA_CURR, ui_get_ina_current_ma, 0, 0},
    {STR_CHG1_CAP, ui_get_temp_cap, 0, 0},
    {STR_DIS_CAP, ui_get_temp_cap, 0, 0},
    {STR_CHG2_CAP, ui_get_temp_cap, 0, 0},
    {STR_ELAP_TIME, ui_get_temp_elap_s, 0, 0}};

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

static void action_toggle_discharge(void)
{
    uint8_t state = g_current_menu->soft_key_state[2];
    if (state == 1u)
    {
        PORTD &= (uint8_t)~(1u << PD5);
    }
    else
    {
        PORTD |= (uint8_t)(1u << PD5);
    }
}

static void action_toggle_charging(void)
{
    uint8_t state = g_current_menu->soft_key_state[2];
    if (state == 1u)
    {
        PORTD |= (uint8_t)(1u << PD6);
    }
    else
    {
        PORTD &= (uint8_t)~(1u << PD6);
    }
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
            action_toggle_discharge();
        }
        return;
    }

    if (menu == m_run_chg)
    {
        /* Items: 1=Start (leaf) */
        if (selected_1based == 1u)
        {
            action_toggle_charging();
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
    /* ---------- MAIN MENU ---------- */
    {
        static const char STR_MAIN0[] PROGMEM = "1.Battery simple test";
        static const char STR_MAIN1[] PROGMEM = "2.Battery cycles test";
        static const char STR_MAIN2[] PROGMEM = "3.Intern resist. test";
        static const char STR_MAIN3[] PROGMEM = "4.View logs files";

        static const menu_context_t MAIN_ROWS[] PROGMEM = {
            {STR_MAIN0, &m_batt_simple_list_obj},
            {STR_MAIN1, &m_run_cycles_obj},
            {STR_MAIN2, &m_run_resist_obj},
            {STR_MAIN3, &m_logs_stub_obj}};

        menu_init(m_main);
        (void)menu_set_menu_rows(m_main, sizeof(MAIN_ROWS) / sizeof(MAIN_ROWS[0]), MAIN_ROWS);
        menu_set_name(m_main, PSTR("   ---Main menu---"));
    }

    /* ---------- Battery simple test (LIST) ---------- */
    {
        static const char STR_BST0[] PROGMEM = "1.Chg-Disch-Chg";
        static const char STR_BST1[] PROGMEM = "2.Charging";
        static const char STR_BST2[] PROGMEM = "3.Discharging";

        static const menu_context_t BST_ROWS[] PROGMEM = {
            {STR_BST0, &m_run_cdc_obj},
            {STR_BST1, &m_run_chg_obj},
            {STR_BST2, &m_run_disch_obj}};

        menu_init(m_batt_simple_list);
        (void)menu_set_menu_rows(m_batt_simple_list, sizeof(BST_ROWS) / sizeof(BST_ROWS[0]), BST_ROWS);
        menu_set_name(m_batt_simple_list, PSTR(" --Btr simple test--"));
        (void)menu_set_soft_keys(m_batt_simple_list, SK_BACK_ONLY);
    }

    /* RUN: Chg-Disch-Chg */
    {
        // static const char *const items[] = {
        //     "Options",
        //     "STOP"};

        // m_run_cdc = menu_create();
        // (void)menu_init_list(m_run_cdc, 2u, items);
        // menu_set_name(m_run_cdc, "Chg-Disch-Chg");
        menu_init(m_run_cdc);
        (void)menu_set_leaf_rows(m_run_cdc, sizeof(INA_LEAF_ROWS) / sizeof(INA_LEAF_ROWS[0]), INA_LEAF_ROWS);
        menu_set_name(m_run_cdc, PSTR("   -Chg-Disch-Chg-"));
        (void)menu_set_soft_keys(m_run_cdc, SK_BACK_OPTIONS_STOP);
    }

    /* OPTIONS: Chg-Disch-Chg-Options (Back only) */
    {
        menu_init(m_opt_cdc);
        // menu_set_name(m_opt_cdc, "Chg-Disch-Chg-Options");
        menu_set_name(m_opt_cdc, PSTR(" -Chg-Disch-Chg-Opts-"));
        (void)menu_set_soft_keys(m_opt_cdc, SK_BACK_ONLY);
        m_opt_cdc->prev_menu = m_run_cdc;
    }

    /* RUN: Discharging */
    {
        // static const char *const items[] = {
        //     "STOP"};

        // m_run_disch = menu_create();
        // (void)menu_init_list(m_run_disch, 1u, items);
        // menu_set_name(m_run_disch, "Discharging");
        menu_init(m_run_disch);
        (void)menu_set_leaf_rows(m_run_disch, sizeof(INA_DIS_LEAF_ROWS) / sizeof(INA_DIS_LEAF_ROWS[0]), INA_DIS_LEAF_ROWS);
        menu_set_name(m_run_disch, PSTR("    -Discharging-"));
        (void)menu_set_soft_keys(m_run_disch, SK_BACK_TOGGLE_DISCH);
    }

    /* RUN: Charging (Finished -> Start) */
    {
        // static const char *const items[] = {
        //     "Start"};

        // m_run_chg = menu_create();
        // (void)menu_init_list(m_run_chg, 1u, items);
        // menu_set_name(m_run_chg, "Charging");
        menu_init(m_run_chg);
        (void)menu_set_leaf_rows(m_run_chg, sizeof(INA_CHG_LEAF_ROWS) / sizeof(INA_CHG_LEAF_ROWS[0]), INA_CHG_LEAF_ROWS);
        menu_set_name(m_run_chg, PSTR("     -Charging-"));
        (void)menu_set_soft_keys(m_run_chg, SK_BACK_TOGGLE_CHG);
    }

    /* ---------- Battery cycles test (RUN) ---------- */
    {
        // static const char *const items[] = {
        //     "Options",
        //     "STOP"};

        // m_run_cycles = menu_create();
        // (void)menu_init_list(m_run_cycles, 2u, items);
        // menu_set_name(m_run_cycles, "Battery cycles test");
        static const char STR_RCYC0[] PROGMEM = " ";
        static const char STR_RCYC1[] PROGMEM = " ";
        static const menu_context_t RCYC_ROWS[] PROGMEM = {
            {STR_RCYC0, &m_opt_cycles_obj},
            {STR_RCYC1, &m_modal_stop_confirm_obj}};

        menu_init(m_run_cycles);
        (void)menu_set_menu_rows(m_run_cycles, sizeof(RCYC_ROWS) / sizeof(RCYC_ROWS[0]), RCYC_ROWS);
        menu_set_name(m_run_cycles, PSTR(" --Btr cycles test--"));
        (void)menu_set_soft_keys(m_run_cycles, SK_BACK_OPTIONS_STOP_CYC);
    }

    /* OPTIONS: Battery test-Options (Back only) */
    {
        menu_init(m_opt_cycles);
        // menu_set_name(m_opt_cycles, "Battery test-Options");
        menu_set_name(m_opt_cycles, PSTR("   -Btr test-Opts-"));
        (void)menu_set_soft_keys(m_opt_cycles, SK_BACK_ONLY);
    }

    /* MODAL: Stop confirm */
    {
        // static const char *const items[] = {
        //     "Sure",
        //     "Cancel"};

        // m_modal_stop_confirm = menu_create();
        // (void)menu_init_list(m_modal_stop_confirm, 2u, items);
        // menu_set_name(m_modal_stop_confirm, "Stop?");
        static const char STR_MSC0[] PROGMEM = "Sure";
        static const char STR_MSC1[] PROGMEM = "Cancel";
        static const menu_context_t MSC_ROWS[] PROGMEM = {
            {STR_MSC0, 0},
            {STR_MSC1, 0}};

        menu_init(m_modal_stop_confirm);
        (void)menu_set_menu_rows(m_modal_stop_confirm, sizeof(MSC_ROWS) / sizeof(MSC_ROWS[0]), MSC_ROWS);
        menu_set_name(m_modal_stop_confirm, PSTR("Stop?"));
        (void)menu_set_soft_keys(m_modal_stop_confirm, 0);
    }

    /* ---------- Resist test (RUN) ---------- */
    {
        // static const char *const items[] = {
        //     "AC/DC",
        //     "START/STOP"};

        // m_run_resist = menu_create();
        // (void)menu_init_list(m_run_resist, 2u, items);
        // menu_set_name(m_run_resist, "Resist. test (DC/AC)");
        static const char STR_RRT0[] PROGMEM = " ";
        static const char STR_RRT1[] PROGMEM = " ";
        static const menu_context_t RRT_ROWS[] PROGMEM = {
            {STR_RRT0, 0},
            {STR_RRT1, 0}};

        menu_init(m_run_resist);
        (void)menu_set_menu_rows(m_run_resist, sizeof(RRT_ROWS) / sizeof(RRT_ROWS[0]), RRT_ROWS);
        menu_set_name(m_run_resist, PSTR(" Resist. test (DC/AC)"));
        (void)menu_set_soft_keys(m_run_resist, SK_BACK_ACDC_STARTSTOP);
    }

    /* ---------- Logs stub ---------- */
    {
        menu_init(m_logs_stub);
        // menu_set_name(m_logs_stub, "Logs");
        menu_set_name(m_logs_stub, PSTR("       -Logs-"));
        (void)menu_set_soft_keys(m_logs_stub, SK_BACK_ONLY);
    }

    /* ---------- Something else stub ---------- */
    {
        menu_init(m_something_stub);
        // menu_set_name(m_something_stub, "Something else");
        menu_set_name(m_something_stub, PSTR("  -Something else-"));
        (void)menu_set_soft_keys(m_something_stub, SK_BACK_ONLY);
    }

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
        (void)menu_on_plus(m);
        return;
    }

    if (btn == UI_BTN_MINUS)
    {
        (void)menu_on_minus(m);
        return;
    }

    /* OK pressed */
    {
        menu_action_t a = menu_on_ok(m);

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
