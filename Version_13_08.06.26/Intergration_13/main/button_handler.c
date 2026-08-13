#include "button_handler.h"
// Global state variables for password input and verification
int entered_password[PASSWORD_LENGTH] = {0};
int current_digit_index = 0;
bool password_verified = false;


//const TickType_t button_debounce_delay = pdMS_TO_TICKS(0);
#define DEBOUNCE_DELAY_MS   30
#define POST_PRESS_DELAY_MS 100

void reset_password_input(void) {
    memset(entered_password, 0, sizeof(entered_password));
    current_digit_index = 0;
}

void initialize_buttons() {
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << BUTTON_1) | (1ULL << BUTTON_2) | 
                       (1ULL << BUTTON_BACK) | (1ULL << BUTTON_UP) | 
                       (1ULL << BUTTON_NEXT) | (1ULL << Silence_BUTTON),
                       
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_ENABLE
    };
    gpio_config(&io_conf);
}

static bool is_button_pressed(gpio_num_t gpio)
{
    if (gpio_get_level(gpio) == 0) {
        vTaskDelay(pdMS_TO_TICKS(DEBOUNCE_DELAY_MS));
        return gpio_get_level(gpio) == 0;
    }
    return false;
}

void handle_button1_action() {
    switch(current_state) {
        case STATE_NAME_OF_SOCIETY:
            show_screen(STATE_MAIN_MENU);
            break;

        case STATE_MAIN_MENU:
            show_screen(STATE_LEVEL1_MENU);
            break;
            
        case STATE_LEVEL1_MENU:
            show_screen(STATE_ACTIVITY_LOG);
            break;
            
        case STATE_LEVEL2_MENU:
            show_screen(STATE_FLAT_OWNER);
            break;

        case STATE_SEND_ALL_LOGS:
            show_screen(STATE_SEND_ACTIVITY_LOGS);
            break;
            
        default:
            break;
    }
}

void handle_button2_action(){
    switch(current_state) {
        case STATE_NAME_OF_SOCIETY:
            show_screen(STATE_MAIN_MENU);
            break;
        case STATE_MAIN_MENU:
            show_screen(STATE_PASSWORD);
            break;
        case STATE_LEVEL1_MENU:
            show_screen(STATE_PASSWORD);
            break;
        case STATE_LEVEL2_MENU:
            show_screen(STATE_SEND_ALL_LOGS);
            break;
        case STATE_SEND_ALL_LOGS:
            show_screen(STATE_SEND_SYSTEM_LOGS);
            break;
        default:
            break;
    }
}

void handle_back_button_action(){
    switch(current_state) {
        case STATE_NAME_OF_SOCIETY:
            show_screen(STATE_MAIN_MENU);
            break;
        case STATE_MAIN_MENU:
            show_screen(STATE_NAME_OF_SOCIETY);
            break;
            
        case STATE_LEVEL1_MENU:
            show_screen(STATE_MAIN_MENU);
            break;
            
        case STATE_LEVEL2_MENU:
            show_screen(STATE_MAIN_MENU);
            break;

        case STATE_ACTIVITY_LOG:
            show_screen(STATE_LEVEL1_MENU);
            break;

        case STATE_PASSWORD:
            show_screen(STATE_MAIN_MENU);
            reset_password_input();
            break;

        case STATE_FLAT_OWNER:
            show_screen(STATE_LEVEL2_MENU);
            break;

        case STATE_SEND_ALL_LOGS:
            show_screen(STATE_LEVEL2_MENU);
            break;
        case STATE_SEND_ACTIVITY_LOGS:
            show_screen(STATE_SEND_ALL_LOGS);
            break;
        case STATE_SEND_SYSTEM_LOGS:
            show_screen(STATE_SEND_ALL_LOGS);
            break;
        default:
            break;
    }
}

void handle_up_button_action(){
    if (current_state == STATE_PASSWORD) {
        entered_password[current_digit_index]++;
        if (entered_password[current_digit_index] > 9)
            entered_password[current_digit_index] = 0;
        show_screen(STATE_PASSWORD);
    }
    else if (current_state == STATE_ACTIVITY_LOG && total_logs > 0) {
        if (current_log_index > 0) {
            current_log_index--;
        } else {
            current_log_index = total_logs - 1;  // wrap to last
        }
        display_log_entry(current_log_index);
        }
    else if (current_state == STATE_FLAT_OWNER && total_owners > 0) {
        current_owner = (current_owner - 1 + total_owners) % total_owners;
        show_owner_slide(current_owner);   // from flash_csv.c
    }
}

void handle_next_button_action() {
    if (current_state == STATE_PASSWORD) {
        current_digit_index++;
        if (current_digit_index >= PASSWORD_LENGTH) {
            current_digit_index = 0;
            bool match = true;
            for (int i = 0; i < PASSWORD_LENGTH; i++) {
                if (entered_password[i] != correct_password[i]) {
                    match = false;
                    break;
                }
            }
            if (match) {
                password_verified = true;
                reset_password_input();
                show_screen(STATE_LEVEL2_MENU);
            } else {
                memset(entered_password, 0, sizeof(entered_password));
                reset_password_input();
                // i2c_lcd_clear();
                // i2c_lcd_set_cursor(1, 2);
                // i2c_lcd_send_string("Wrong Password");
                // vTaskDelay(pdMS_TO_TICKS(1500));

                lcd_display_text(1, 2, "Wrong Password", true);
                vTaskDelay(pdMS_TO_TICKS(1500));
                show_screen(STATE_PASSWORD);                
            }
        }else {
            show_screen(STATE_PASSWORD);
        }
    }

    else if (current_state == STATE_ACTIVITY_LOG && total_logs > 0) {
        if (current_log_index < total_logs - 1) {
            current_log_index++;
        } else {
            current_log_index = 0;  // wrap to first
        }
        display_log_entry(current_log_index);
    }

    else if (current_state == STATE_FLAT_OWNER && total_owners > 0) {
        current_owner = (current_owner + 1) % total_owners;
        show_owner_slide(current_owner);   // from flash_csv.c
    }
}

// ================= Silence Button Handler =================
void handle_silence_button(void) {
    shouldBeep = false;
    gpio_set_level(BUZZER_PIN, 0);
    printf("Buzzer silenced - Buttons Activated\n");
}

void button_task(void *arg) {
    while (1) {
        // Check SILENCE button FIRST - ALWAYS WORKS even when shouldBeep is true
        if (is_button_pressed(Silence_BUTTON)) {
            lcd_register_activity();
            handle_silence_button();
            vTaskDelay(pdMS_TO_TICKS(POST_PRESS_DELAY_MS));
        }

        // Check OTHER 5 BUTTONS only if shouldBeep is FALSE
        if (!shouldBeep) {
            // BUTTON 1
            if (is_button_pressed(BUTTON_1)) {
                lcd_register_activity();
                handle_button1_action();
            }

            // BUTTON 2
            if (is_button_pressed(BUTTON_2)) {
                lcd_register_activity();
                handle_button2_action();
            }

            // BUTTON BACK
            if (is_button_pressed(BUTTON_BACK)) {
                lcd_register_activity();
                handle_back_button_action();
            }

            // BUTTON UP
            if (is_button_pressed(BUTTON_UP)) {
                lcd_register_activity();
                handle_up_button_action();
            }

            // BUTTON NEXT
            if (is_button_pressed(BUTTON_NEXT)) {
                lcd_register_activity();
                handle_next_button_action();
            }
            vTaskDelay(pdMS_TO_TICKS(POST_PRESS_DELAY_MS));
        } 
        vTaskDelay(pdMS_TO_TICKS(10)); // Polling interval
    }
}