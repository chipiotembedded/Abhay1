#include "system_init.h"
void system_initialize() {
    i2c_lcd_init();
    i2c_lcd_backlight(true);
    initialize_buttons();
    initialize_activity_log();
    //usb_update_mount_littlefs();
    flash_csv_init();
    //load_owners_from_csv();        // load last saved owners from flash
    //usb_update_start();    
    
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    app_rtc_init();  // Start RTC task

}