#ifndef BUTTON_HANDLER_H
#define BUTTON_HANDLER_H

#define BUTTON_1 4
#define BUTTON_2 5
#define BUTTON_BACK 6
#define BUTTON_UP 7      
#define BUTTON_NEXT 17
//#define Silence_BUTTON 18

#include <string.h>
#include "freertos/FreeRTOS.h"   // For TickType_t
#include "freertos/task.h" 
#include "lcd_i2c.h"
#include "screen_display.h"
#include "system_init.h"
#include "button_handler.h"
#include "driver/gpio.h"
#include "usb_update.h"
#include "password.h"


extern const TickType_t button_debounce_delay;

void initialize_buttons();
void button_task(void *arg);
void reset_password_input(void);
void handle_button1_action();
void handle_button2_action();
void handle_back_button_action();
void handle_up_button_action();
void handle_next_button_action();
// void handle_silence_button(void);
//void display_log_entry(int index);

#endif