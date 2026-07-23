#ifndef BUTTON_HANDLER_H
#define BUTTON_HANDLER_H

#include "lcd_i2c.h"
#include "screen_display.h"
#include "system_init.h"
#include "system_states.h"
#include "freertos/FreeRTOS.h"   // For TickType_t
#include "freertos/task.h" 
#include <string.h>

extern const TickType_t button_debounce_delay;

void initialize_buttons();
void handle_button_press(int button);
void button_task(void *arg);

void reset_password_input(void);
void handle_button1_action();
void handle_button2_action();
void handle_back_button_action();
void handle_up_button_action();
void handle_next_button_action();
void handle_silence_button(void);
void handle_button_press(int button);
//void display_log_entry(int index);

#endif