/*
///////    WITHOUT MAC    /////
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "lora.h"
#include "esp_sleep.h"
#define TAG "LORA"
#define BUTTON_PIN 4
#define MSG "Flat no 101, Atharva"


// creating a variable to queue 
    static QueueHandle_t queue =  NULL;

// ============ ISR ========

    static void IRAM_ATTR button_isr_handler(void *arg){
        
        uint32_t gpio_num = (uint32_t) arg;
        xQueueSendFromISR(queue, &gpio_num, NULL);
      
    }
    

    static void tx_task(void *arg) {
        uint32_t io_num;
        while (1)
        {
            if (xQueueReceive(queue, &io_num, portMAX_DELAY)){
                ESP_LOGI(TAG, "Button pressed, Sending packet..");
                    lora_init();
                     lora_set_bandwidth  (125000);
                     lora_set_coding_rate (5); 
                     lora_set_frequency (865000000);
                     lora_set_sync_word (0XF3);
                     lora_set_spreading_factor (11);
                     lora_set_tx_power (17);
                     lora_enable_crc();
    
                lora_send_packet((uint8_t*) MSG, strlen(MSG));
                ESP_LOGI(TAG, "Packet Sent : %s", MSG);
                 lora_sleep();
                 ESP_LOGI(TAG, "Lora in sleep mode..!\n");
                ESP_LOGI(TAG, "Esp going in deep_sleep\n");
                vTaskDelay(pdMS_TO_TICKS(100));
                 esp_deep_sleep_start();
            }
        }
    }


void app_main() {

    ESP_LOGI(TAG,"Lora Starting..! ");


    gpio_config_t gpio = {
    gpio.mode = GPIO_MODE_INPUT,
    gpio.intr_type = GPIO_INTR_NEGEDGE,
    gpio.pin_bit_mask = (1ULL << BUTTON_PIN),
    gpio.pull_down_en = GPIO_PULLDOWN_DISABLE ,
    gpio.pull_up_en = GPIO_PULLUP_ENABLE,
    };
    gpio_config(&gpio);
    esp_sleep_enable_ext0_wakeup(BUTTON_PIN, 0);
    

    // create  stack 
    
    queue = xQueueCreate(10,sizeof(uint32_t));
    // install interrupt 

    gpio_install_isr_service(0);
    gpio_isr_handler_add(BUTTON_PIN,button_isr_handler, (void*)BUTTON_PIN );

   // xTaskCreate(flags, "Flags",4096, NULL, 10, NULL);
    xTaskCreate(tx_task, "Tx_task", 4096, NULL, 10, NULL);
    
    esp_sleep_wakeup_cause_t wakeup = esp_sleep_get_wakeup_cause();
    if (wakeup == ESP_SLEEP_WAKEUP_EXT0){
        ESP_LOGI(TAG, "Woke up from button pressed..!\n");
        uint32_t gpio_num = BUTTON_PIN;
        xQueueSend(queue, &gpio_num, 0);
    }
    else {
        ESP_LOGI(TAG, "Wake from reset.\n");
    }
    ESP_LOGI(TAG, "Going into deep sleep..!\n");
    vTaskDelay(pdMS_TO_TICKS(3000));
    esp_deep_sleep_start();

    ESP_LOGI(TAG, "Setup completed. button pressed on gpio %d to send lora_packet.\n", BUTTON_PIN);
}
*/

///////    WITH MAC    /////
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "lora.h"
#include "esp_sleep.h"
#include "esp_mac.h"
#include "esp_timer.h"

#define TAG "MAC_SENDER"
#define BUTTON_PIN 4

// creating a variable to queue 
static QueueHandle_t queue =  NULL;
//static volatile int64_t last_press_time = 0;  // microseconds

// ============ ISR ========    
static void IRAM_ATTR button_isr_handler(void *arg){
        
        uint32_t gpio_num = (uint32_t) arg;
        xQueueSendFromISR(queue, &gpio_num, NULL);
      
    }

// ================== Send MAC ==================
static void send_mac_once(void) {
    uint8_t mac[6];
    char mac_str[18];

    // Read device MAC (station MAC)
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    snprintf(mac_str, sizeof(mac_str), "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    // Send over LoRa
    lora_send_packet((uint8_t *)mac_str, strlen(mac_str));
    ESP_LOGI(TAG, "MAC Sent: %s", mac_str);
}

    static void tx_task(void *arg) {
        uint32_t io_num;
        while (1)
        {
            if (xQueueReceive(queue, &io_num, portMAX_DELAY)){
                ESP_LOGI(TAG, "Button pressed, Sending packet..");
                    
                send_mac_once();

                ESP_LOGI(TAG, "Lora in sleep mode..!\n");
                //vTaskDelay(pdMS_TO_TICKS(50));  // let logs flush
                //lora_sleep();
                lora_close();
                ESP_LOGI(TAG, "Esp going in deep_sleep\n");
                vTaskDelay(pdMS_TO_TICKS(100));
                esp_deep_sleep_start();
            }
        }
    }


void app_main() {

    ESP_LOGI(TAG,"Lora Starting..! ");


    gpio_config_t gpio = {
        .pin_bit_mask = (1ULL << BUTTON_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_NEGEDGE
    };

    gpio_config(&gpio);

    // Allow wakeup from button
    esp_sleep_enable_ext0_wakeup(BUTTON_PIN, 0);
    
    lora_init();
    lora_set_bandwidth(250000);
    lora_set_coding_rate(5); 
    lora_set_frequency(865000000);
    lora_set_sync_word(0xF3);
    lora_set_spreading_factor(12);
    lora_set_tx_power(17);
    lora_enable_crc();

    // create  stack 
    queue = xQueueCreate(10,sizeof(uint32_t));
    // install interrupt 
    gpio_install_isr_service(0);
    gpio_isr_handler_add(BUTTON_PIN,button_isr_handler, (void*)BUTTON_PIN );

   // xTaskCreate(flags, "Flags",4096, NULL, 10, NULL);
    xTaskCreate(tx_task, "Tx_task", 4096, NULL, 10, NULL);
    
    esp_sleep_wakeup_cause_t wakeup = esp_sleep_get_wakeup_cause();
    if (wakeup == ESP_SLEEP_WAKEUP_EXT0){
        ESP_LOGI(TAG, "Woke up from button pressed..!\n");
        uint32_t gpio_num = BUTTON_PIN;
        xQueueSend(queue, &gpio_num, 0);
    }
    else {
        ESP_LOGI(TAG, "Wake from reset.\n");
    }
        vTaskDelay(pdTICKS_TO_MS(100));
    ESP_LOGI(TAG, "Esp going in deep_sleep\n");
    //lora_sleep();
    lora_close();
    esp_deep_sleep_start();
    ESP_LOGI(TAG, "Setup completed. button pressed on gpio %d to send lora_packet.\n", BUTTON_PIN);
}

