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
// void request_screen(StateID state)
// {
//     current_state = state;
//     show_screen(state);
// }

void request_screen(StateID state)
{
    if (display_queue != NULL) {
        // Send to queue, don't call directly
        xQueueSend(display_queue, &state, pdMS_TO_TICKS(100));
    } else {
        // Fallback if queue not ready (only during initialization)
        current_state = state;
        if (lcd_mutex) {
            xSemaphoreTake(lcd_mutex, portMAX_DELAY);
            show_screen(state);
            xSemaphoreGive(lcd_mutex);
        }
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
    {STATE_SEND_ACTLOG_TO_USB, "Send ActLog"},
    // {STATE_SEND_LOG_TO_USB, "Send LOG"},
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
            vTaskDelay(pdMS_TO_TICKS(50));
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

        // case STATE_SEND_ACTLOG_TO_USB:
        //     if (copy_activity_to_usb() == ESP_OK && copy_log_to_usb() == ESP_OK && github_upload_log() == ESP_OK) {
        //         i2c_lcd_clear();
        //         i2c_lcd_set_cursor(0,0);
        //         i2c_lcd_send_string("Log Exported");
        //     } else {
        //         i2c_lcd_clear();
        //         i2c_lcd_set_cursor(0,0);
        //         i2c_lcd_send_string("Export Failed");
        //     }
        //     break;

        case STATE_SEND_ACTLOG_TO_USB: {
            /*
             * This state is intentionally handled OUTSIDE lcd_mutex in
             * display_task() above.  All LCD calls here use lcd_safe_*
             * to avoid dead-locks.
             */
            // {
            //     const char *lines[] = {"Exporting...", NULL, NULL, NULL};

            //     i2c_lcd_send_string("Exporting...");
            // }

            i2c_lcd_clear();
            i2c_lcd_set_cursor(0,3);
            i2c_lcd_send_string("Exporting");

            bool ok_act = (copy_activity_to_usb() == ESP_OK);
            bool ok_log = (copy_log_to_usb()      == ESP_OK);
            bool ok_git = (github_upload_log()    == ESP_OK);
            bool ok_act_git = (github_upload_activity_log()    == ESP_OK);

            i2c_lcd_clear();
            if (ok_act && ok_log && ok_git && ok_act_git) {
                //const char *lines[] = {"Export OK", "Git: OK", NULL, NULL};
                i2c_lcd_set_cursor(3,0);
                i2c_lcd_send_string("Export OK  Git: OK");
            } else {
                // char fail[17] = {0};
                // snprintf(fail, sizeof(fail), "A:%d L:%d G:%d AG:%d",
                //          ok_act, ok_log, ok_git, ok_act_git);
                // const char *lines[] = {"Export PARTIAL", fail, NULL, NULL};
                i2c_lcd_set_cursor(3,0);
                i2c_lcd_send_string("failed");
            }
            vTaskDelay(pdMS_TO_TICKS(100));
            break;
        } 

        default:
            i2c_lcd_set_cursor(0, 0);
            i2c_lcd_send_string("Invalid State");
            break;
    }
    //xSemaphoreGive(lcd_mutex);
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
