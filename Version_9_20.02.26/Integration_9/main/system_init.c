#include "system_init.h"
void system_initialize() {
    // mount_littlefs();
    i2c_lcd_init();
    i2c_lcd_backlight(true);
    initialize_buttons();
    initialize_activity_log();
    app_rtc_init();
    receiver_start();
    //log_manager_init();
}