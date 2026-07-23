Integration_9

In this version added a functionality of copy all system logs to littlefs.
copy_log_to_usb is the function which copies system log files from littlefs to usb.
This function was triggred through menu functionality (Under level 2--->Send ActLog).

Using 2 functions on 1 state (STATE_SEND_ACTLOG_TO_USB), copy_log_to_usb function was called.
case STATE_SEND_ACTLOG_TO_USB:
            if (copy_activity_to_usb() == ESP_OK && copy_log_to_usb() == ESP_OK) {
                i2c_lcd_clear();
                i2c_lcd_set_cursor(0,0);
                i2c_lcd_send_string("Log Exported");
            } else {
                i2c_lcd_clear();
                i2c_lcd_set_cursor(0,0);
                i2c_lcd_send_string("Export Failed");
            }
            break;


Also made mount_littlefs() as the first function to initialize after app_main().
Made log_manager_init() as 2nd function to initialize.
This was required , so that all the system logs would get stored in littlefs right from 1st initialization.
This was done by making esp_err_t mount_littlefs(void).....which will report success or failure of that function.

Also remember there should be no return function inside a task like mount_littlefs inside usb_update_task.
example-
void usb_update_task(void *arg) {
    ESP_LOGI(TAG, "USB update task started");

    // MOUNT LITTLEFS FIRST - This was missing!
     if (!mount_littlefs()) {
         ESP_LOGE(TAG, "Failed to mount LittleFS, USB update disabled");
         return;
     }

    // Create event queue
    app_queue = xQueueCreate(5, sizeof(app_message_t));

    // Start USB task
    xTaskCreate(usb_task, "usb_task", 4096, NULL, 2, NULL);

   