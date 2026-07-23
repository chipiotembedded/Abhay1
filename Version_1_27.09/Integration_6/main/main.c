#include "system_states.h"
#include "screen_display.h"
#include "button_handler.h"
#include "system_init.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "lora_receive.h"
#include "activity_log.h"
#include "flash_csv.h"

static const char *TAG = "Main";
static QueueHandle_t display_queue = NULL;

// Global state variables
StateID current_state = STATE_BOOT;
int entered_password[PASSWORD_LENGTH] = {0};
int current_digit_index = 0;
const int correct_password[PASSWORD_LENGTH] = {1, 1, 1, 1};
bool password_verified = false;

void display_task(void *arg) {
    StateID id;
    while (1) {
        if (xQueueReceive(display_queue, &id, portMAX_DELAY)) {
            current_state = id;
            show_screen(id);  // ✅ Add this line to reflect on LCD
        }
    }
}

void app_main(void) {
    // Initialize all system components
    ESP_LOGI(TAG, "System initialization started");
    system_initialize();
    receiver_start();

    
    // if (flash_csv_mount() == ESP_OK) {
    //     load_owners_from_csv();   // load last saved owners from flash
    // }

    //xTaskCreate(flat_owner_nav_task, "FlatOwnerNav", 2048, NULL, 2, NULL);
    
    // Create display queue
    display_queue = xQueueCreate(10, sizeof(StateID));
    if(display_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create display queue");
        return;
    }
    
    // Create tasks
    if(xTaskCreate(display_task, "Display", 4096, NULL, 5, NULL) != pdPASS) {
        ESP_LOGE(TAG, "Failed to create display task");
        return;
    }
    
    if(xTaskCreate(button_task, "Buttons", 4096, NULL, 5, NULL) != pdPASS) {
        ESP_LOGE(TAG, "Failed to create button task");
        return;
    }
    
    ESP_LOGI(TAG, "Starting application");
    
    // Initial screen sequence
    request_screen(STATE_BOOT);
    vTaskDelay(pdMS_TO_TICKS(1000));
    request_screen(STATE_NAME_OF_SOCIETY);
}