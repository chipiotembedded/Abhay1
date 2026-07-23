#include "system_init.h"
void system_initialize() {
    i2c_lcd_init();

    //flash_all_defaults();
    
    load_owners_from_csv();
    load_pass_from_txt();
    load_wifi_cred_from_txt();

    initialize_buttons();
    initialize_activity_log();
    receiver_start();
}
