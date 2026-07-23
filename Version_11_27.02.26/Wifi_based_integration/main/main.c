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

static const char *TAG = "Main";
QueueHandle_t display_queue = NULL;

// Global state variables
// StateID current_state = STATE_BOOT;
int entered_password[PASSWORD_LENGTH] = {0};
int current_digit_index = 0;
//const int correct_password[PASSWORD_LENGTH] = {1, 1, 1, 1};
bool password_verified = false;


void app_main(void) {   
    // ========== ADD THIS ==========
    // Initialize watchdog with 30 second timeout
    // esp_task_wdt_config_t twdt_config = {
    //     .timeout_ms = 60000,  // 30 seconds in milliseconds
    //     .idle_core_mask = (1 << 0) | (1 << 1),  // Monitor both cores
    //     .trigger_panic = true,  // Panic on timeout (auto-reboot)
    // };
    // esp_task_wdt_init(&twdt_config);
    //esp_task_wdt_add(NULL);        // Add current task (main) to watchdog

    // Create task handles to track them
    // TaskHandle_t button_handle = NULL;
    // TaskHandle_t display_handle = NULL;
    // TaskHandle_t usb_handle = NULL;
    // TaskHandle_t lcd_handle = NULL;

    ESP_LOGI(TAG, "Step 1: Mounting LittleFS");
    mount_littlefs();
    
    ESP_LOGI(TAG, "Step 2: Log manager init");
    log_manager_init();

    // Initialize all system components
    ESP_LOGI(TAG, "Step 3: System initialize");
    system_initialize();

    if (xSemaphoreTake(lcd_mutex, pdMS_TO_TICKS(20))){
        i2c_lcd_clear();
        i2c_lcd_set_cursor(0,0);
        i2c_lcd_send_string("Plz Wait for");
        i2c_lcd_set_cursor(1,0);
        i2c_lcd_send_string("Wifi and Time");
        xSemaphoreGive(lcd_mutex);
    }

  
    ESP_LOGI(TAG, "Step 4: Waiting for WiFi");
    wifi_manager_init();
    wifi_manager_wait_for_connection();

    if (wifi_manager_is_connected()){
        if (xSemaphoreTake(lcd_mutex, pdMS_TO_TICKS(20))){
            i2c_lcd_clear();
            i2c_lcd_set_cursor(0,0);
            i2c_lcd_send_string("WiFi Connected");  
            xSemaphoreGive(lcd_mutex);
        }
        vTaskDelay(pdMS_TO_TICKS(2000));
        wifi_rtc_init();
    }else{
        if (xSemaphoreTake(lcd_mutex, pdMS_TO_TICKS(20))){
            i2c_lcd_clear();
            i2c_lcd_set_cursor(0,0);
            i2c_lcd_send_string("Wifi failed");
            xSemaphoreGive(lcd_mutex);
        }
    } 
    vTaskDelay(pdMS_TO_TICKS(2000));

    // Create display queue
    display_queue = xQueueCreate(10, sizeof(StateID));
    if(display_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create display queue");
        return;
    }
    
    // Create tasks
    if(xTaskCreate(display_task, "Display", 8192, NULL, 8, NULL) != pdPASS) {
        ESP_LOGE(TAG, "Failed to create display task");
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

    // Add tasks to watchdog
    // if (display_handle) esp_task_wdt_add(display_handle);
    // if (button_handle) esp_task_wdt_add(button_handle);
    // if (usb_handle) esp_task_wdt_add(usb_handle);
    // if (lcd_handle) esp_task_wdt_add(lcd_handle);
    
    //vTaskDelay(pdMS_TO_TICKS(1000));

    ESP_LOGI(TAG, "Starting application");
    
    // Initial screen sequence
    request_screen(STATE_BOOT);
    vTaskDelay(pdMS_TO_TICKS(1000));
    request_screen(STATE_NAME_OF_SOCIETY);
}