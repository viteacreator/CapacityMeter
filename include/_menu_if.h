// #ifndef MENU_IF_H
// #define MENU_IF_H

// #include "main.h"
// #include "menu.h"


// Menu_t main_menu; // Menu item definitions
// const char mainMenuItems[][MENU_MAX_ITEM_NAME_LEN] = {
//     "Measure",
//     "Settings",
//     "History",
//     "Exit"};

// Menu_t settings_menu; // Submenu for settings
// const char settingsMenuItems[][MENU_MAX_ITEM_NAME_LEN] = {
//     "Voltage",
//     "Current",
//     "Back"};

// void initMenus()
// {
//   root_menu = &main_menu; // Set root menu
//   // Initialize main menu
//   init_menu(&main_menu, 4, (char (*)[MENU_MAX_ITEM_NAME_LEN])mainMenuItems);
//   set_menu_name(&main_menu, "Main Menu");
//   select_item(&main_menu, 1); // Default to first item

//   // Initialize submenus if needed
//   init_menu(&settings_menu, 3, (char(*)[MENU_MAX_ITEM_NAME_LEN])settingsMenuItems);
//   set_menu_name(&settings_menu, "Settings");
// }

// void print_menu(Menu_t *curr_menu)
// {
//   display.clearDisplay();
//   display.setTextSize(1);
//   display.setTextColor(SSD1306_WHITE);
//   display.setCursor(0, 0);

//   // Display menu name
//   display.println(get_menu_name(curr_menu));
//   display.println("-------------------");

//   // Display items with selection indicator
//   for (int i = 1; i <= get_max_items(curr_menu); i++)
//   {
//     if (i == get_menu_state(curr_menu))
//     {
//       display.print("> "); // Selection indicator
//     }
//     else
//     {
//       display.print("  ");
//     }
//     display.println(get_item_name(curr_menu, i));
//   }

//   display.display();
// }

// #endif // MENU_IF_H