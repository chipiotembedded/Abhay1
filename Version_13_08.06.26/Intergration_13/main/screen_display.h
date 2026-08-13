#ifndef SCREEN_DISPLAY_H
#define SCREEN_DISPLAY_H

#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include "lcd_i2c.h"
#include "activity_log.h"
#include "usb_update.h"
#include "password.h"
#include "system_init.h"
#include "log_manager.h"
#include "git_hub_uploader.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// Screen states
typedef enum {
    STATE_BOOT,
    STATE_NAME_OF_SOCIETY,
    STATE_MAIN_MENU,
    STATE_LEVEL1_MENU,
    STATE_LEVEL2_MENU,
    STATE_ACTIVITY_LOG,
    STATE_FLAT_OWNER,
    STATE_PASSWORD,
    STATE_COUNT,
    STATE_SEND_ALL_LOGS,
    STATE_SEND_ACTIVITY_LOGS,
    STATE_SEND_SYSTEM_LOGS

} StateID;

extern StateID current_state;

void show_screen(StateID state);

#endif