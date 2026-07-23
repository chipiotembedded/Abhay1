#ifndef SYSTEM_INIT_H
#define SYSTEM_INIT_H

#include "lcd_i2c.h"
#include "button_handler.h"
#include "screen_display.h"
#include "rtc.h"
#include "activity_log.h"
#include "usb_update.h"
#include "button_handler.h"
#include "log_manager.h"
#include "password.h"
#include "wifi_manager.h"

/**
 * @brief Initializes all system components
 */
void system_initialize(void);

#endif