#include "system_init.h"
void system_initialize() {
    i2c_lcd_init();
    i2c_lcd_backlight(true);
    initialize_buttons();
    initialize_activity_log();
    app_rtc_init();
    
}