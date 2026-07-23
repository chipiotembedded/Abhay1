#include "screen_display.h"
static const char *TAG = "SCREEN_DISPLAY";
// static FlatOwner current_received;

StateID current_state = STATE_BOOT;

/*
void request_screen(StateID state) {
    if (state < STATE_COUNT) {
        current_state = state;
        show_screen(state);
    }
}
*/
void request_screen(StateID state)
{
    // if (state >= STATE_COUNT) return;
    // /* Prevent unnecessary redraw */
    // if (current_state == state) return;
    current_state = state;
    show_screen(state);
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
    {STATE_SEND_TO_USB, "Send ActLog"},
};

void display_task(void *arg)
{
    StateID id;

    ESP_LOGI(TAG, "Display task started");

    while (1) {
        if (xQueueReceive(display_queue, &id, portMAX_DELAY) == pdTRUE) {

            current_state = id;

            /* LCD access must be protected */
            if (lcd_mutex) {
                xSemaphoreTake(lcd_mutex, portMAX_DELAY);
                show_screen(id);
                xSemaphoreGive(lcd_mutex);
            } else {
                ESP_LOGE(TAG, "LCD mutex not initialized!");
            }
        }
    }
}

void show_screen(StateID state) {
    /* HARD GUARD: mutex must exist */
    if (lcd_mutex == NULL) {
        ESP_LOGE(TAG, "lcd_mutex NULL");
        return;
    }

    /* BLOCK until LCD is free */
    xSemaphoreTake(lcd_mutex, portMAX_DELAY);

    i2c_lcd_clear();
    
    switch(state) {
        case STATE_BOOT:
            i2c_lcd_set_cursor(0, 3);
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

        // case STATE_ACTIVITY_LOG:
        //     debug_print_all_logs();
        //     if (xSemaphoreTake(nvs_mutex, portMAX_DELAY) == pdTRUE) {
        //         total_logs = get_total_logs();
        //         if (total_logs > 0) {
        //             current_log_index = 0;
        //             display_log_entry(current_log_index);
        //         } else {
        //             i2c_lcd_set_cursor(0, 0);
        //             i2c_lcd_send_string("No activity logs");
        //             i2c_lcd_set_cursor(1, 0);
        //             i2c_lcd_send_string("available");
        //         }
        //         xSemaphoreGive(nvs_mutex);
        //     }
        //     break;

        case STATE_ACTIVITY_LOG: {
            total_logs = get_total_logs();   // Directly read from file

            if (total_logs > 0) {
                current_log_index = 0;
                display_log_entry(current_log_index);
            } else {
                i2c_lcd_clear();
                i2c_lcd_set_cursor(0, 0);
                i2c_lcd_send_string("No activity logs");
            }
            break;
        }

        case STATE_FLAT_OWNER:
            vTaskDelay(pdMS_TO_TICKS(50));
            ESP_LOGI("SCREEN", "Entering Flat Owner screen");
            if (total_owners > 0) {
                current_owner = 0;
                show_owner_slide(current_owner);
            } else {
                i2c_lcd_set_cursor(1, 0);
                i2c_lcd_send_string("No owners found");
            }
            break;

        case STATE_PASSWORD:
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

        case STATE_SEND_TO_USB:
            if (copy_activity_to_usb() == ESP_OK) {
                i2c_lcd_clear();
                i2c_lcd_set_cursor(0,0);
                i2c_lcd_send_string("Log Exported");
            } else {
                i2c_lcd_clear();
                i2c_lcd_set_cursor(0,0);
                i2c_lcd_send_string("Export Failed");
            }
            break;

        default:
            i2c_lcd_set_cursor(0, 0);
            i2c_lcd_send_string("Invalid State");
            break;
    }
    xSemaphoreGive(lcd_mutex);
}

/*
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
*/

void show_menu(const MenuItem *menu, size_t count)
{
    i2c_lcd_set_cursor(0, 0);
    i2c_lcd_send_string("Menu");

    for (int i = 0; i < count && i < 3; i++) {
        char line[20];
        snprintf(line, sizeof(line), "%d. %s", i + 1, menu[i].name);
        i2c_lcd_set_cursor(i + 1, 0);
        i2c_lcd_send_string(line);
    }
}
