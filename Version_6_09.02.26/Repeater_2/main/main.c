#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "lora.h"

#define TAG "LoRa_RX"
#define SUPPRESSION_WINDOW 5000  // ms

static char last_mac[18] = {0};
static int64_t last_mac_time = 0;
static bool last_mac_valid = false;

static bool should_accept(const char *mac) {
    int64_t now = esp_timer_get_time() / 1000; // ms

    if(last_mac_valid && strcasecmp(last_mac,mac) ==0 && (now-last_mac_time)< SUPPRESSION_WINDOW){
        ESP_LOGW(TAG,"Duplicate MAC suppressed: %s", mac);
        return false;
    }

    //Update last mac
    strncpy(last_mac,mac,sizeof(last_mac));
    last_mac[sizeof(last_mac) - 1] = '\0';
    last_mac_time = now;
    last_mac_valid = true;

    return true;
}

void lora_rx_task(void *pvParameter)
{
    uint8_t buf[27];
    //uint8_t buff[26];
    int len;
   
    ESP_LOGI(TAG, "LoRa Receiver Task Started");

    // Initialize LoRa
    lora_init();
    lora_set_bandwidth  (250000);
    lora_set_coding_rate (5); 
    lora_set_frequency (865000000);
    lora_set_sync_word (0XF3);
    lora_set_spreading_factor (12);
    lora_set_tx_power (17);
    lora_enable_crc();            // Put LoRa in RX continuous mode
    lora_receive();

    while (1) {
        len = lora_receive_packet(buf, sizeof(buf)-1);
        if (len == 17) {
            buf[len] = 0; // Null-terminate for printing
            ESP_LOGI(TAG, "Received: %s (len=%d)", (char*)buf, len);
           
            if (should_accept((char*)buf)){
                ESP_LOGI(TAG, "Forwarding MAC: %s", (char *)buf);
                lora_send_packet(buf,len);
            }
            lora_receive();
        }
        vTaskDelay(pdMS_TO_TICKS(10)); // Small delay to yield CPU
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "Starting LoRa RX Example...");
    xTaskCreate(&lora_rx_task, "lora_rx_task", 4096, NULL, 5, NULL);
   
}
