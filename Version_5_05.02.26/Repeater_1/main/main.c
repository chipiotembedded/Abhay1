#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "lora.h"

#define TAG "LoRa_RX"
#define MAX_SENDERS 100
#define SUPPRESSION_WINDOW 5000  // ms

typedef struct {
    char mac[18];
    int64_t last_seen;   // timestamp in ms
} SenderEntry;

static SenderEntry sender_table[MAX_SENDERS];
static int sender_count = 0;

static bool should_accept(const char *mac) {
    int64_t now = esp_timer_get_time() / 1000; // ms

    for (int i = 0; i < sender_count; i++) {
        if (strcasecmp(sender_table[i].mac, mac) == 0) {
            if (now - sender_table[i].last_seen < SUPPRESSION_WINDOW) {
                ESP_LOGW(TAG, "Duplicate suppressed: %s", mac);
                return false;
            } else {
                sender_table[i].last_seen = now;
                return true;
            }
        }
    }

    // New sender → add entry
    if (sender_count < MAX_SENDERS) {
        strncpy(sender_table[sender_count].mac, mac, sizeof(sender_table[0].mac));
        sender_table[sender_count].mac[sizeof(sender_table[0].mac) - 1] = '\0';
        sender_table[sender_count].last_seen = now;
        sender_count++;
    }
    return true;
}

void lora_rx_task(void *pvParameter)
{
    uint8_t buf[26];
    
    uint8_t buff[26];

    
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
        
        len = lora_receive_packet(buf, sizeof(buf));
        if (len > 0) {
            buf[len] = 0; // Null-terminate for printing
            ESP_LOGI(TAG, "Received: %s (len=%d)", (char*)buf, len);
           
            if (should_accept((char*)buf)){
            //     for (int i =  0 ; i < sizeof(buf); i++){
            //         buff[i] =  buf[i];
            //     }
                memcpy(buff, buf, len);
                vTaskDelay(pdTICKS_TO_MS(10));
                // printf("Packet sending %s\n len =  %d\n" , (char *)buff, len);
                ESP_LOGI(TAG, "Forwarding MAC: %s", (char *)buff);
                lora_send_packet(buff,len);
                // memset(buff, 0, sizeof(buff));
                vTaskDelay(pdTICKS_TO_MS(50));
                lora_receive();

            }
        }
    vTaskDelay(pdMS_TO_TICKS(100)); // Small delay to yield CPU
    
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "Starting LoRa RX Example...");
    xTaskCreate(&lora_rx_task, "lora_rx_task", 4096, NULL, 5, NULL);
   
}
