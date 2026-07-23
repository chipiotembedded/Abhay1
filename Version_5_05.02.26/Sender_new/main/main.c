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

// ================== Send MAC ==================
static void send_mac_once(void)
{
    uint8_t mac[6];
    char mac_str[18];

    esp_read_mac(mac, ESP_MAC_WIFI_STA);

    snprintf(mac_str, sizeof(mac_str),
             "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2],
             mac[3], mac[4], mac[5]);

    ESP_LOGI(TAG, "Sending MAC: %s", mac_str);

    lora_send_packet((uint8_t *)mac_str, strlen(mac_str));

    // Ensure TX completion before sleep
    vTaskDelay(pdMS_TO_TICKS(200));
}

void app_main(void)
{
    ESP_LOGI(TAG, "Booting...");

    // Configure button
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << BUTTON_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);

    // Enable EXT0 wakeup
    esp_sleep_enable_ext0_wakeup(BUTTON_PIN, 0);

    // Init LoRa
    lora_init();
    lora_set_bandwidth(250000);
    lora_set_coding_rate(5); 
    lora_set_frequency(865000000);
    lora_set_sync_word(0xF3);
    lora_set_spreading_factor(12);
    lora_set_tx_power(17);
    lora_enable_crc();

    esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();

    if (cause == ESP_SLEEP_WAKEUP_EXT0) {
        ESP_LOGI(TAG, "Button wakeup → Sending MAC");
        send_mac_once();

        // VERY IMPORTANT: wait for TX to finish  
        vTaskDelay(pdMS_TO_TICKS(200));
    } else {
        ESP_LOGI(TAG, "Going to sleep");
    }

    lora_debug_status();
    lora_sleep();
    lora_debug_status();
    ESP_LOGI(TAG, "Entering deep sleep");
    esp_deep_sleep_start();
}
       