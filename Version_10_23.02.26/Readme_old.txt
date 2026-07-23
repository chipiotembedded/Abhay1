In this Version,
Primarily i picked modular files made by Siddhesh J.
Those were git_hub_uploader.c and git_hub_uploader.h
The above consisted of rtc and wifi based functions. Also that rtc was taking time from github.

The functions for uploading on git [github_upload_log()] was called from screens_display.c...using STATE_SEND_ACTLOG_TO_USB
case STATE_SEND_ACTLOG_TO_USB:
            if (copy_activity_to_usb() == ESP_OK && copy_log_to_usb() == ESP_OK && github_upload_log() == ESP_OK) {
                i2c_lcd_clear();
                i2c_lcd_set_cursor(0,0);
                i2c_lcd_send_string("Log Exported");
            } else {
                i2c_lcd_clear();
                i2c_lcd_set_cursor(0,0);
                i2c_lcd_send_string("Export Failed");
            }
            break;

In the starting of app_main...mount_littlefs() and log_manager_init() were called.


Due to many functions being called from app_main function, it gave stack overflow error.
To fix this i increased Size of factory partition from 0x100000 (1M) to 0x180000 & offset of storage partition from 0x110000 to  0x190000.
		factory, app, factory, 0x10000, 1M, 
		storage, data, littlefs, 0x110000, 0x80000,

			TO

		factory,  app,  factory, 0x10000, 0x180000,
		storage,  data, littlefs, 0x190000,  0x80000,


Finally i created rtc.c and wifi_manager.c modular files.
Removed all wifi and rtc related functions from git_hub_uploader.c file
Called following functions in main:
		mount_littlefs();  
    		log_manager_init();
    		wifi_manager_init();
    		wifi_manager_wait_for_connection();

    		if (wifi_manager_is_connected())
   		 {
        			wifi_rtc_init();
    		}


Also remeber donot name wifi_rtc_init() as rtc_init()...because it clash with internal files named as rtc_init().

Added lcd string (Sending to Git) while functioning of github_upload_log.

In this project Wi-Fi credentials are hardcoded