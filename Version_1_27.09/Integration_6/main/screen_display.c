#include "screen_display.h"
#include "lcd_i2c.h"
#include "button_handler.h"
#include "system_states.h"
#include "activity_log.h"
#include "flash_csv.h"
#include <string.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// int entered_password[PASSWORD_LENGTH] = {0};
// int current_digit_index = 0;
// static StateID current_state = STATE_BOOT;
// static QueueHandle_t display_queue;

void request_screen(StateID state) {
    if (state < STATE_COUNT) {
        current_state = state;
        show_screen(state);
    }
}

const MenuItem main_menu_items[] = {
    {STATE_LEVEL1_MENU, "Level 1"},
    {STATE_LEVEL2_MENU, "Level 2"},
    //{STATE_LEVEL3_MENU, "Level 3"}
};

const MenuItem level1_menu_items[] = {
    {STATE_ACTIVITY_LOG, "Activity Log"},
    {STATE_LEVEL2_MENU, "Go to Level 2"}
};

const MenuItem level2_menu_items[] = {
    {STATE_FLAT_OWNER, "Flat Owners"},
    {STATE_UPDATE_INFO, "Update info"}
};

// const MenuItem level3_menu_items[] = {
//     {STATE_ACTIVITY_LOG, "Installation Mode"}
// };

void show_screen(StateID state) {
    i2c_lcd_clear();
    
    switch(state) {
        case STATE_BOOT:
            const char* welcome_frames[] = { ". ", "..", "..." };
            for (int i = 0; i < 3; i++) {
                i2c_lcd_clear();
                i2c_lcd_set_cursor(0, 0);
                i2c_lcd_send_string("Welcome");
                i2c_lcd_send_string(welcome_frames[i]);
                vTaskDelay(pdMS_TO_TICKS(1000));
            }
            break;
            
        case STATE_NAME_OF_SOCIETY:
            i2c_lcd_set_cursor(0, 0);
            i2c_lcd_send_string("Name of Building");
            i2c_lcd_set_cursor(1, 0);
            i2c_lcd_send_string("Building No. 123");
            i2c_lcd_set_cursor(2, 0);
            i2c_lcd_send_string("Wing-A");
            break;
            
        case STATE_MAIN_MENU:
            show_menu(main_menu_items, sizeof(main_menu_items)/sizeof(MenuItem));
            break;
            
        case STATE_LEVEL1_MENU:
            show_menu(level1_menu_items, sizeof(level1_menu_items)/sizeof(MenuItem));
            break;
            
        case STATE_LEVEL2_MENU:
            show_menu(level2_menu_items, sizeof(level2_menu_items)/sizeof(MenuItem));
            break;

        // case STATE_LEVEL3_MENU:
        //     show_menu(level3_menu_items, sizeof(level3_menu_items)/sizeof(MenuItem));
        //     break;

        case STATE_ACTIVITY_LOG:
            debug_print_all_logs();
            if (xSemaphoreTake(nvs_mutex, portMAX_DELAY) == pdTRUE) {
                total_logs = get_total_logs();
                if (total_logs > 0) {
                    // if (current_log_index >= total_logs) {
                    //     current_log_index = total_logs - 1;
                    // }
                    current_log_index = 0;
                    display_log_entry(current_log_index);
                } else {
                    i2c_lcd_set_cursor(0, 0);
                    i2c_lcd_send_string("No activity logs");
                    i2c_lcd_set_cursor(1, 0);
                    i2c_lcd_send_string("available");
                }
                xSemaphoreGive(nvs_mutex);
            }
            break;

        case STATE_FLAT_OWNER:
            ESP_LOGI("SCREEN", "Entering Flat Owner screen");
            //load_owners_from_csv();                     // Load owners from flash
            vTaskDelay(pdMS_TO_TICKS(50));
            current_owner = 0;
            show_owner_slide(current_owner);  // Show first owner
            break;

        case STATE_PASSWORD:
            i2c_lcd_clear();
            i2c_lcd_set_cursor(0, 3);
            i2c_lcd_send_string("Enter PIN:");

            char line[PASSWORD_LENGTH + 1] = {0};
            for (int i = 0; i < PASSWORD_LENGTH; i++) {
                if (i == current_digit_index)
                    snprintf(&line[i], 2, "%d", entered_password[i]);  // Show current digit
                else
                    snprintf(&line[i], 2, "*");                        // Mask others
            }

            i2c_lcd_set_cursor(1, 4);
            i2c_lcd_send_string(line);
            break;

        case STATE_UPDATE_INFO:
            // i2c_lcd_clear();
            // i2c_lcd_set_cursor(0, 0);
            // i2c_lcd_send_string("Waiting for USB...");
            //usb_update_mount_littlefs();  // Call once at startup
            //usb_update_start(); 
            break;

        case STATE_INSTALLATION_MODE:
            i2c_lcd_clear();
            i2c_lcd_set_cursor(0, 0);
            i2c_lcd_send_string("MAC ID");
            break;

        default:
            // i2c_lcd_set_cursor(0, 0);
            // i2c_lcd_send_string("Invalid Screen");
            break;
    }
}

void show_menu(const MenuItem *menu, size_t count) {
    i2c_lcd_clear();
    i2c_lcd_set_cursor(0, 0);
    i2c_lcd_send_string("Menu");
    
    for(int i = 0; i < count && i < 3; i++) {
        i2c_lcd_set_cursor(i + 1, 0);
        char line[20];
        snprintf(line, sizeof(line), "%d. %s", i + 1, menu[i].name);
        i2c_lcd_send_string(line);
    }
}