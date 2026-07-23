
#include "screen_display.h"
#include "button_handler.h"
#include "system_init.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "lora_receive.h"
#include "activity_log.h"
#include "usb_update.h"
#include "password.h"
#include "git_hub_uploader.h"
#include "wifi_manager.h"
#include "rtc.h"
#include "esp_task_wdt.h"
#include "lcd_i2c.h"

static const char *TAG = "Main";

// Global state variables
// int entered_password[PASSWORD_LENGTH] = {0};
// int current_digit_index = 0;
// bool password_verified = false;

void app_main(void) {   
    printf(TAG, "Starting application");
    printf(TAG, "Step 1: Mounting LittleFS");
    mount_littlefs();
    
    printf(TAG, "Step 2: Log manager init");
    log_manager_init();

    // Initialize all system components
    printf(TAG, "Step 3: System initialize");
    system_initialize();

    // Create LCD queue
    printf(TAG, "Step 4: LCD Task initialize");
    lcd_queue = xQueueCreate(20, sizeof(lcd_message_t));
    if (lcd_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create LCD queue");
        return;
    }
    if(xTaskCreate(lcd_task, "lcd_task", 8192, NULL, 8, NULL) != pdPASS) {
        ESP_LOGE(TAG, "Failed to create LCD display task");
        return;
    }

    printf(TAG, "Step 5: Backlight Task initialize");
    if (xTaskCreate(lcd_backlight_timeout_task, "lcd_bl_timer", 4096, NULL, 5, NULL) != pdPASS) {
        ESP_LOGE(TAG, "Failed to create lcd_bl_timer task");
    } else {
        ESP_LOGI(TAG, "lcd_bl_timer task created successfully");
    }

    printf(TAG, "Step 6: Button Task initialize");
    if(xTaskCreate(button_task, "Buttons", 8192, NULL, 5, NULL) != pdPASS) {
        ESP_LOGE(TAG, "Failed to create button task");
        return;
    }



    lcd_display_text(0, 0, "Plz Wait for", true);
    lcd_display_text(1, 0, "Wifi and Time", false);

    printf(TAG, "Step 7: Waiting for WiFi");
    wifi_manager_init();
    wifi_manager_wait_for_connection();

    if (wifi_manager_is_connected()){
        lcd_display_text(0, 0, "Wifi Connected", true);
        lcd_display_text(1, 0, "Wait for Time", false);
        vTaskDelay(pdMS_TO_TICKS(1000));
        wifi_rtc_init();
    } else {
        lcd_display_text(0, 0, "Wifi Failed", true);
    } 


    printf(TAG, "Step 8: USB Task initialize");
    if (xTaskCreate(usb_update_task, "usb_update_task", 4096, NULL, 8, NULL) != pdPASS) {
        ESP_LOGE(TAG, "Failed to create USB update task");
    } else {
        ESP_LOGI(TAG, "USB update task created successfully");
    }    

    vTaskDelay(pdMS_TO_TICKS(2000));
    // Initial screen sequence
    show_screen(STATE_BOOT);
    vTaskDelay(pdMS_TO_TICKS(1000));
    show_screen(STATE_NAME_OF_SOCIETY);
    // i2c_lcd_backlight(true);
}
