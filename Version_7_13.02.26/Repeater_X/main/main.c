#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "lora.h"

#define MY_ID "X"
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
    uint8_t buf[100];
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
        if (len > 0) {
            buf[len] = 0; // Null-terminate for printing
            ESP_LOGI(TAG, "Received: %s (len=%d)", (char*)buf, len);
           
            char temp[100];
            strcpy(temp, (char*)buf);
            char *sender = strtok(temp, "|");

            if (!sender) {
                lora_receive();
                continue;
            }

            if (should_accept(sender)){
                if (strstr((char*)buf, MY_ID) != NULL) {
                    ESP_LOGW(TAG, "Packet already passed through me. Dropping.");
                    lora_receive();
                    continue;
                }

                // Append my ID
                // strcat((char*)buf, "|");
                // strcat((char*)buf, MY_ID);
                if (strlen((char*)buf) + strlen(MY_ID) + 1 < sizeof(buf)) {
                    strcat((char*)buf, "|");
                    strcat((char*)buf, MY_ID);

                    ESP_LOGI(TAG, "Forwarding: %s", (char*)buf);
                    lora_send_packet(buf, strlen((char*)buf));
                } else {
                    ESP_LOGW(TAG, "Buffer full. Dropping packet.");
                    lora_receive();
                    continue;
                }
            }
            lora_receive();
        }
        vTaskDelay(pdMS_TO_TICKS(100)); // Small delay to yield CPU
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "Starting LoRa RX Example...");
    xTaskCreate(&lora_rx_task, "lora_rx_task", 4096, NULL, 5, NULL);
   
}
