#ifndef SCREEN_DISPLAY_H
#define SCREEN_DISPLAY_H

#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include "lcd_i2c.h"
#include "button_handler.h"
#include "activity_log.h"
#include "usb_update.h"
#include "password.h"
#include "system_init.h"
#include "log_manager.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// Screen states
typedef enum {
    STATE_BOOT,
    STATE_NAME_OF_SOCIETY,
    STATE_MAIN_MENU,
    STATE_LEVEL1_MENU,
    STATE_LEVEL2_MENU,
    STATE_LEVEL3_MENU,
    STATE_ACTIVITY_LOG,
    STATE_FLAT_OWNER,
    STATE_PASSWORD,
    STATE_UPDATE_INFO,
    STATE_INSTALLATION_MODE,
    STATE_COUNT,
    STATE_SEND_ACTLOG_TO_USB

} StateID;

// Menu item structure
typedef struct {
    StateID state;
    const char* name;
} MenuItem;

extern const MenuItem main_menu_items[];
extern const MenuItem level1_menu_items[];
extern const MenuItem level2_menu_items[];
extern const MenuItem level3_menu_items[];

extern QueueHandle_t display_queue;
extern StateID current_state;

void show_screen(StateID state);
void display_task(void *arg);
void show_menu(const MenuItem *menu, size_t count);
void request_screen(StateID state);
#endif