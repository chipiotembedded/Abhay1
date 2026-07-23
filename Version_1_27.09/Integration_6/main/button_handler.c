#include "button_handler.h"
#include "screen_display.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

const TickType_t button_debounce_delay = pdMS_TO_TICKS(0);

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

void handle_button1_action() {
    switch(current_state) {
        case STATE_NAME_OF_SOCIETY:
            request_screen(STATE_MAIN_MENU);
            vTaskDelay(button_debounce_delay);
            break;

        case STATE_MAIN_MENU:
            request_screen(STATE_LEVEL1_MENU);
            vTaskDelay(button_debounce_delay);
            break;
            
        case STATE_LEVEL1_MENU:
            request_screen(STATE_ACTIVITY_LOG);
            vTaskDelay(button_debounce_delay);
            break;
            
        case STATE_LEVEL2_MENU:
            request_screen(STATE_FLAT_OWNER);
            vTaskDelay(button_debounce_delay);
            break;
            
        case STATE_LEVEL3_MENU:
            request_screen(STATE_INSTALLATION_MODE);
            vTaskDelay(button_debounce_delay);
            break;
        default:
            // i2c_lcd_set_cursor(0, 0);
            // i2c_lcd_send_string("Invalid Screen");
            break;
    }
}

void handle_button2_action(){
    switch(current_state) {
        case STATE_NAME_OF_SOCIETY:
            request_screen(STATE_MAIN_MENU);
            vTaskDelay(button_debounce_delay);
            break;
        case STATE_MAIN_MENU:
            request_screen(STATE_PASSWORD);
            vTaskDelay(button_debounce_delay);
            break;
            
        case STATE_LEVEL1_MENU:
            request_screen(STATE_PASSWORD);
            vTaskDelay(button_debounce_delay);
            break;
            
        case STATE_LEVEL2_MENU:
            request_screen(STATE_UPDATE_INFO);
            vTaskDelay(button_debounce_delay);
            break;
            
        case STATE_LEVEL3_MENU:
            request_screen(STATE_INSTALLATION_MODE);
            vTaskDelay(button_debounce_delay);
            break;
        default:
            // i2c_lcd_set_cursor(0, 0);
            // i2c_lcd_send_string("Invalid Screen");
            break;
    }
}

void handle_back_button_action(){
    switch(current_state) {
        case STATE_NAME_OF_SOCIETY:
            request_screen(STATE_MAIN_MENU);
            vTaskDelay(button_debounce_delay);
            break;
        case STATE_MAIN_MENU:
            request_screen(STATE_NAME_OF_SOCIETY);
            vTaskDelay(button_debounce_delay);
            break;
            
        case STATE_LEVEL1_MENU:
            request_screen(STATE_MAIN_MENU);
            vTaskDelay(button_debounce_delay);
            break;
            
        case STATE_LEVEL2_MENU:
            request_screen(STATE_MAIN_MENU);
            vTaskDelay(button_debounce_delay);
            break;
            
        case STATE_LEVEL3_MENU:
            request_screen(STATE_MAIN_MENU);
            vTaskDelay(button_debounce_delay);
            break;

        case STATE_ACTIVITY_LOG:
            request_screen(STATE_LEVEL1_MENU);
            vTaskDelay(button_debounce_delay);
            break;

        case STATE_PASSWORD:
            request_screen(STATE_MAIN_MENU);
            vTaskDelay(button_debounce_delay);
            break;

        case STATE_FLAT_OWNER:
            request_screen(STATE_LEVEL2_MENU);
            vTaskDelay(button_debounce_delay);
            break;

        case STATE_UPDATE_INFO:
            request_screen(STATE_LEVEL2_MENU);
            vTaskDelay(button_debounce_delay);
            break;

        case STATE_INSTALLATION_MODE:
            request_screen(STATE_LEVEL3_MENU);
            vTaskDelay(button_debounce_delay);
            break;
            
        default:
            // i2c_lcd_set_cursor(0, 0);
            // i2c_lcd_send_string("Invalid Screen");
            break;
    }
}

void handle_up_button_action(){
    if (current_state == STATE_PASSWORD) {
        entered_password[current_digit_index]++;
        if (entered_password[current_digit_index] > 9)
            entered_password[current_digit_index] = 0;
        request_screen(STATE_PASSWORD);
        vTaskDelay(button_debounce_delay);
    }
    else if (current_state == STATE_ACTIVITY_LOG && total_logs > 0) {
        // current_log_index = (current_log_index - 1 + total_logs) % total_logs;
        if (current_log_index > 0) {
            current_log_index--;
        } else {
            current_log_index = total_logs - 1;  // wrap to last
        }
        display_log_entry(current_log_index);
        vTaskDelay(button_debounce_delay);
        }
    else if (current_state == STATE_FLAT_OWNER && total_owners > 0) {
        current_owner = (current_owner - 1 + total_owners) % total_owners;
        show_owner_slide(current_owner);   // from flash_csv.c
        vTaskDelay(button_debounce_delay);
    }
    //break;
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
                request_screen(STATE_LEVEL2_MENU);
            } else {
                memset(entered_password, 0, sizeof(entered_password));
                reset_password_input();
                i2c_lcd_clear();
                i2c_lcd_set_cursor(1, 2);
                i2c_lcd_send_string("Wrong Password");
                vTaskDelay(pdMS_TO_TICKS(1500));
                request_screen(STATE_PASSWORD);                
            }
        }else {
            request_screen(STATE_PASSWORD);
        }
        vTaskDelay(button_debounce_delay);
    }

    else if (current_state == STATE_ACTIVITY_LOG && total_logs > 0) {
        if (current_log_index < total_logs - 1) {
            current_log_index++;
        } else {
            current_log_index = 0;  // wrap to first
        }
        display_log_entry(current_log_index);
        vTaskDelay(button_debounce_delay);
    }

    else if (current_state == STATE_FLAT_OWNER && total_owners > 0) {
        current_owner = (current_owner + 1) % total_owners;
        show_owner_slide(current_owner);   // from flash_csv.c
        vTaskDelay(button_debounce_delay);
    }
}

// ================= Silence Button Handler =================
void handle_silence_button(void) {
    shouldBeep = false;
    gpio_set_level(BUZZER_PIN, 0);
    printf("Buzzer silenced\n");
}
    // Optional LCD message
//     i2c_lcd_clear();
//     i2c_lcd_set_cursor(0, 0);
//     i2c_lcd_send_string("Buzzer Silenced");



void button_task(void *arg) {
    while (1) {
        if (gpio_get_level(BUTTON_1) == 0) {
            vTaskDelay(pdMS_TO_TICKS(50));
            if (gpio_get_level(BUTTON_1) == 0) {
                handle_button1_action();
                vTaskDelay(pdMS_TO_TICKS(200));
            }
        }

        if (gpio_get_level(BUTTON_2) == 0) {
            vTaskDelay(pdMS_TO_TICKS(50));
            if (gpio_get_level(BUTTON_2) == 0) {
                handle_button2_action();
                vTaskDelay(pdMS_TO_TICKS(200));
            }
        }

        if (gpio_get_level(BUTTON_BACK) == 0) {
            vTaskDelay(pdMS_TO_TICKS(50));
            if (gpio_get_level(BUTTON_BACK) == 0) {
                handle_back_button_action();
                vTaskDelay(pdMS_TO_TICKS(200));
            }
        }

        if (gpio_get_level(BUTTON_UP) == 0) {
            vTaskDelay(pdMS_TO_TICKS(50));
            if (gpio_get_level(BUTTON_UP) == 0) {
                handle_up_button_action();
                vTaskDelay(pdMS_TO_TICKS(200));
            }
        }

        if (gpio_get_level(BUTTON_NEXT) == 0) {
            vTaskDelay(pdMS_TO_TICKS(50));
            if (gpio_get_level(BUTTON_NEXT) == 0) {
                handle_next_button_action();
                vTaskDelay(pdMS_TO_TICKS(200));
            }
        }

        if (gpio_get_level(Silence_BUTTON) == 0) {
            vTaskDelay(pdMS_TO_TICKS(50));
            if (gpio_get_level(Silence_BUTTON) == 0) {
                handle_silence_button();
                vTaskDelay(pdMS_TO_TICKS(200));
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10)); // Polling interval
    }
}

void handle_button_press(int button) {
    switch(button) {
        case BUTTON_1:
            handle_button1_action();
            vTaskDelay(button_debounce_delay);
            break;
            
        case BUTTON_2:
            handle_button2_action();
            vTaskDelay(button_debounce_delay);
            break;
            
        case BUTTON_BACK:
            handle_back_button_action();
            vTaskDelay(button_debounce_delay);
            break;
        
        case BUTTON_UP:
            handle_up_button_action();
            vTaskDelay(button_debounce_delay);
            break;

        case BUTTON_NEXT:
            handle_next_button_action();
            vTaskDelay(button_debounce_delay);
            break;

        case Silence_BUTTON:
            handle_silence_button();
            vTaskDelay(button_debounce_delay);
            break;

        default:
            // i2c_lcd_set_cursor(0, 0);
            // i2c_lcd_send_string("Invalid Screen");
            break;
    }
}