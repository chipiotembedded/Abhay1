#include "screen_display.h"

void request_screen(StateID state) {
    if (state < STATE_COUNT) {
        current_state = state;
        show_screen(state);
    }
}

const MenuItem main_menu_items[] = {
    {STATE_LEVEL1_MENU, "Level 1"},
    {STATE_LEVEL2_MENU, "Level 2"},
};

const MenuItem level1_menu_items[] = {
    {STATE_ACTIVITY_LOG, "Activity Log"},
    {STATE_LEVEL2_MENU, "Go to Level 2"}
};

const MenuItem level2_menu_items[] = {
    {STATE_FLAT_OWNER, "Flat Owners"},
};

void show_screen(StateID state) {
    i2c_lcd_clear();
    
    switch(state) {
        case STATE_BOOT:
            i2c_lcd_clear();
            i2c_lcd_set_cursor(0, 4);
            i2c_lcd_send_string("Welcome!");
            const char* Dot_frames[] = { ". ", "..", "..." };
            for (int i = 0; i < 3; i++) {
                i2c_lcd_set_cursor(2, 0);
                i2c_lcd_send_string("Plz Wait");
                i2c_lcd_send_string(Dot_frames[i]);
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

        case STATE_ACTIVITY_LOG:
            debug_print_all_logs();
            if (xSemaphoreTake(nvs_mutex, portMAX_DELAY) == pdTRUE) {
                total_logs = get_total_logs();
                if (total_logs > 0) {
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
            vTaskDelay(pdMS_TO_TICKS(50));
            ESP_LOGI("SCREEN", "Entering Flat Owner screen");
            if (total_owners > 0) {
                current_owner = 0;
                show_owner_slide(current_owner);
            } else {
                i2c_lcd_clear();
                i2c_lcd_set_cursor(1, 0);
                i2c_lcd_send_string("No owners found");
            }
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

        default:
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