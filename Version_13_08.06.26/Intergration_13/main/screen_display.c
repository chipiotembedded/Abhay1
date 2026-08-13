#include "screen_display.h"

StateID current_state = STATE_BOOT;

void show_screen(StateID state) {
    //i2c_lcd_clear();
    current_state = state;
    switch(state) {
        case STATE_BOOT:
            lcd_display_text(0, 3, "Welcome!", true);
            const char* Dot_frames[] = { ". ", "..", "..." };
            for (int i = 0; i < 3; i++) {
                lcd_display_text(2, 0, "Plz Wait", false);
                //i2c_lcd_send_string(Dot_frames[i]);
                vTaskDelay(pdMS_TO_TICKS(10));
                lcd_display_text(2, 9, Dot_frames[i], false);
                vTaskDelay(pdMS_TO_TICKS(1000));
            }
            break;
            
        case STATE_NAME_OF_SOCIETY:
            lcd_display_text(0, 0, "Name of Building", true);
            lcd_display_text(1, 0, "Building No. 123", false);
            lcd_display_text(2, 0, "Wing-A", false);
            break;
            
        case STATE_MAIN_MENU:
            // show_menu(main_menu_items, sizeof(main_menu_items)/sizeof(MenuItem));
            lcd_display_text(0, 0, "Menu", true);
            lcd_display_text(1, 0, "1. Level 1", false);
            lcd_display_text(2, 0, "2. Level 2", false);
            break;
            
        case STATE_LEVEL1_MENU:
            lcd_display_text(0, 0, "Menu", true);
            lcd_display_text(1, 0, "1. Activity Log", false);
            lcd_display_text(2, 0, "2. Go to Level 2", false);
            break;
            
        case STATE_LEVEL2_MENU:
            lcd_display_text(0, 0, "Menu", true);
            lcd_display_text(1, 0, "1. Flat Owners", false);
            lcd_display_text(2, 0, "2. Send All Logs", false);
            break;

        case STATE_ACTIVITY_LOG: {
            total_logs = get_total_logs();   // Directly read from file

            if (total_logs > 0) {
                current_log_index = 0;
                display_log_entry(current_log_index);
            } else {
                lcd_display_text(0, 0, "No Activity Log", true);
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
                lcd_display_text(1, 0, "No Owners Found", true);
            }
            break;

        case STATE_PASSWORD:
            vTaskDelay(pdMS_TO_TICKS(50));
            lcd_display_text(0, 3, "Enter Pin:", true);

            char line[PASSWORD_LENGTH + 1] = {0};
            for (int i = 0; i < PASSWORD_LENGTH; i++) {
                if (i == current_digit_index)
                    snprintf(&line[i], 2, "%d", entered_password[i]);  // Show current digit
                else
                    snprintf(&line[i], 2, "*");                        // Mask others
            }

            lcd_display_text(1, 4, line, false);
            break;

        case STATE_SEND_ALL_LOGS: {
            lcd_display_text(0, 0, "Menu", true);
            lcd_display_text(1, 0, "1. Send Act Logs", false);
            lcd_display_text(2, 0, "2. Send Sys Logs", false);
            break;
        } 

        case STATE_SEND_ACTIVITY_LOGS:
            lcd_display_text(0, 3, "Exporting", true);

            bool ok_act = (copy_activity_to_usb() == ESP_OK);
            bool ok_act_git = (github_upload_activity_log() == ESP_OK);
            
            if (ok_act && ok_act_git) {
                lcd_display_text(0, 0, "USB: Done", true);
                lcd_display_text(1, 0, "Git: Done", false);
            } else if (!ok_act && ok_act_git) {
                lcd_display_text(0, 0, "USB: Failed", true);
                lcd_display_text(1, 0, "Git: Done", false);
            }else if (ok_act && !ok_act_git){
                lcd_display_text(0, 0, "USB: Done", true);
                lcd_display_text(1, 0, "Git: Failed", false);
            }else{
                lcd_display_text(0, 0, "USB: Failed", true);
                lcd_display_text(1, 0, "Git: Failed", false);
            }
            vTaskDelay(pdMS_TO_TICKS(100));            
            break;

        case STATE_SEND_SYSTEM_LOGS:
            lcd_display_text(0, 3, "Exporting", true);

            bool ok_log = (copy_log_to_usb() == ESP_OK);
            bool ok_git = (github_upload_log() == ESP_OK);
            
            if (ok_log && ok_git) {
                lcd_display_text(0, 0, "USB: Done", true);
                lcd_display_text(1, 0, "Git: Done", false);
            } else if (!ok_log && ok_git) {
                lcd_display_text(0, 0, "USB: Failed", true);
                lcd_display_text(1, 0, "Git: Done", false);
            }else if (!ok_log && ok_git){
                lcd_display_text(0, 0, "USB: Done", true);
                lcd_display_text(1, 0, "Git: Failed", false);
            }else{
                lcd_display_text(0, 0, "USB: Failed", true);
                lcd_display_text(1, 0, "Git: Failed", false);
            }
            vTaskDelay(pdMS_TO_TICKS(100));
            break;
        default:
            lcd_display_text(0, 0, "Invalid State", true);
            break;
    }
    //xSemaphoreGive(lcd_mutex);
}