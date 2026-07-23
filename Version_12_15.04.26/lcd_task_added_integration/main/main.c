
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
int entered_password[PASSWORD_LENGTH] = {0};
int current_digit_index = 0;
bool password_verified = false;

/*
void app_main(void) {   
    ESP_LOGI(TAG, "Starting application");
    ESP_LOGI(TAG, "Step 1: Mounting LittleFS");
    mount_littlefs();
    
    ESP_LOGI(TAG, "Step 2: Log manager init");
    log_manager_init();

    // Initialize all system components
    ESP_LOGI(TAG, "Step 3: System initialize");
    system_initialize();

    //vTaskDelay(pdMS_TO_TICKS(2000));

    // Create LCD queue
    lcd_queue = xQueueCreate(20, sizeof(lcd_message_t));
    if(xTaskCreate(lcd_task, "lcd_task", 4096, NULL, 8, NULL) != pdPASS) {
        ESP_LOGE(TAG, "Failed to create LCD display task");
        return;
    }

    if(xTaskCreate(button_task, "Buttons", 8192, NULL, 5, NULL) != pdPASS) {
        ESP_LOGE(TAG, "Failed to create button task");
        return;
    }

    if (xTaskCreate(usb_update_task, "usb_update_task", 4096, NULL, 10, NULL) != pdPASS) {
        ESP_LOGE(TAG, "Failed to create USB update task");
    }else {
        ESP_LOGI(TAG, "USB update task created successfully");
    }

    if (xTaskCreate(lcd_backlight_timeout_task, "lcd_bl_timer", 4096, NULL, 5, NULL) != pdPASS) {
        ESP_LOGE(TAG, "Failed to create lcd_bl_timer task");
    }else {
        ESP_LOGI(TAG, "lcd_bl_timer task created successfully");
    }

    i2c_lcd_backlight(true);
    
    lcd_display_text(0, 0, "Plz Wait for", true);
    lcd_display_text(1, 0, "Wifi and Time", false);

    vTaskDelay(pdMS_TO_TICKS(2000));
    ESP_LOGI(TAG, "Step 4: Waiting for WiFi");
    wifi_manager_init();
    wifi_manager_wait_for_connection();

    if (wifi_manager_is_connected()){
        lcd_display_text(0, 0, "Wifi Connected", true);
        //vTaskDelay(pdMS_TO_TICKS(2000));
        wifi_rtc_init();
        //vTaskDelay(pdMS_TO_TICKS(2000));
    }else{
        lcd_display_text(0, 0, "Wifi Failed", true);
    } 

    vTaskDelay(pdMS_TO_TICKS(2000));

    // Initial screen sequence
    show_screen(STATE_BOOT);
    vTaskDelay(pdMS_TO_TICKS(1000));
    show_screen(STATE_NAME_OF_SOCIETY);
}
*/


void app_main(void) {   
    ESP_LOGI(TAG, "Starting application");
    ESP_LOGI(TAG, "Step 1: Mounting LittleFS");
    mount_littlefs();
    
    ESP_LOGI(TAG, "Step 2: Log manager init");
    log_manager_init();

    // Initialize all system components
    ESP_LOGI(TAG, "Step 3: System initialize");
    system_initialize();

    // Create LCD queue
    lcd_queue = xQueueCreate(20, sizeof(lcd_message_t));
    if (lcd_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create LCD queue");
        return;
    }
    if(xTaskCreate(lcd_task, "lcd_task", 8192, NULL, 8, NULL) != pdPASS) {
        ESP_LOGE(TAG, "Failed to create LCD display task");
        return;
    }
    // if(xTaskCreatePinnedToCore(lcd_task, "lcd_task", 8192, NULL, 8, NULL, 0)!= pdPASS) {
    //     ESP_LOGE(TAG, "Failed to create LCD display task");
    //     return;
    // }

    receiver_start();

    if (xTaskCreate(lcd_backlight_timeout_task, "lcd_bl_timer", 4096, NULL, 5, NULL) != pdPASS) {
        ESP_LOGE(TAG, "Failed to create lcd_bl_timer task");
    } else {
        ESP_LOGI(TAG, "lcd_bl_timer task created successfully");
    }
    // if (xTaskCreatePinnedToCore(lcd_backlight_timeout_task, "lcd_bl_timer", 4096, NULL, 5, NULL, 0)!= pdPASS) {
    //     ESP_LOGE(TAG, "Failed to create lcd_bl_timer task");
    // } else {
    //     ESP_LOGI(TAG, "lcd_bl_timer task created successfully");
    // }

    if(xTaskCreate(button_task, "Buttons", 8192, NULL, 5, NULL) != pdPASS) {
        ESP_LOGE(TAG, "Failed to create button task");
        return;
    }
    // if(xTaskCreatePinnedToCore(button_task, "Buttons", 8192, NULL, 5, NULL, 0)!= pdPASS) {
    //     ESP_LOGE(TAG, "Failed to create button task");
    //     return;
    // }

    if (xTaskCreate(usb_update_task, "usb_update_task", 4096, NULL, 8, NULL) != pdPASS) {
        ESP_LOGE(TAG, "Failed to create USB update task");
    } else {
        ESP_LOGI(TAG, "USB update task created successfully");
    }
    // if (xTaskCreatePinnedToCore(usb_update_task, "usb_update_task", 4096, NULL, 8, NULL, 0)!= pdPASS) {
    //     ESP_LOGE(TAG, "Failed to create USB update task");
    // } else {
    //     ESP_LOGI(TAG, "USB update task created successfully");
    // }

    lcd_display_text(0, 0, "Plz Wait for", true);
    lcd_display_text(1, 0, "Wifi and Time", false);

    ESP_LOGI(TAG, "Step 4: Waiting for WiFi");
    wifi_manager_init();
    wifi_manager_wait_for_connection();

    if (wifi_manager_is_connected()){
        lcd_display_text(0, 0, "Wifi Connected", true);
        vTaskDelay(pdMS_TO_TICKS(1000));
        wifi_rtc_init();
    } else {
        lcd_display_text(0, 0, "Wifi Failed", true);
    } 

    vTaskDelay(pdMS_TO_TICKS(2000));
    // Initial screen sequence
    show_screen(STATE_BOOT);
    vTaskDelay(pdMS_TO_TICKS(1000));
    show_screen(STATE_NAME_OF_SOCIETY);
    // i2c_lcd_backlight(true);
}
