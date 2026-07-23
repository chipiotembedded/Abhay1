#include "lora_receive.h"

#define TAG "LORA_RECEIVE"
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

// ================= Panic System Flags =================
// bool shouldBeep = false;

// ================= Buzzer Task =================
// void buzzer_control_task(void *pvParameter) {
//     gpio_config_t io_conf = {
//         .pin_bit_mask = (1ULL << BUZZER_PIN),
//         .mode = GPIO_MODE_OUTPUT,
//         .pull_up_en = GPIO_PULLUP_DISABLE,
//         .pull_down_en = GPIO_PULLDOWN_DISABLE,
//         .intr_type = GPIO_INTR_DISABLE
//     };
//     gpio_config(&io_conf);
//     gpio_set_level(BUZZER_PIN, 0);

//     while (1) {
//         gpio_set_level(BUZZER_PIN, shouldBeep ? 1 : 0);
//         vTaskDelay(pdMS_TO_TICKS(10));
//     }
// }

FlatOwner* find_owner_by_mac(const char *mac) {
    for (int i = 0; i < total_owners; i++) {
        if (strcasecmp(owners[i].mac, mac) == 0) {
            return &owners[i];
        }
    }
    return NULL; // not found
}

static void lora_receive_task(void *param){
    uint8_t buf[256];
    int len;

    lora_receive();
    ESP_LOGI(TAG, "LoRa Receiver Task Started");
    while (1) {
        len = lora_receive_packet(buf, sizeof(buf));
        if (len > 0) {
            //buf[len] = 0; // Null-terminate for printing
            if (len >= sizeof(buf)) 
            len = sizeof(buf) - 1;
            buf[len] = '\0'; // safe string termination
            lcd_register_activity();
            
            ESP_LOGI(TAG, "Received: %s (len=%d)", (char*)buf, len);

            if (should_accept((char*)buf)) {
                FlatOwner *owner = find_owner_by_mac((char*)buf);
                if (owner) {
                    //shouldBeep = true;
                    i2c_lcd_clear();
                    i2c_lcd_set_cursor(0, 0);
                    i2c_lcd_send_string(owner->name);
                    i2c_lcd_set_cursor(1, 0);
                    i2c_lcd_send_string(owner->flat);

                    ESP_LOGI(TAG, "Owner Found: %s, Flat %s", owner->name, owner->flat);
                    log_received_data(false, owner->name, owner->flat);

                    char timestr[32]; 
                    rtc_get_time_str(timestr, sizeof(timestr));
                    ESP_LOGI(TAG, "Received at: %s", timestr);

                } else {
                    i2c_lcd_clear();
                    i2c_lcd_set_cursor(0, 0);
                    i2c_lcd_send_string("Unknown MAC");
                    i2c_lcd_set_cursor(1, 0);
                    i2c_lcd_send_string((char*)buf);

                    ESP_LOGW(TAG, "MAC not in CSV: %s", (char*)buf);
                }
            // } else {
            //     // just skip duplicates
            }
            lora_receive();
        }
        vTaskDelay(pdMS_TO_TICKS(100)); // Small delay to yield CPU
    }
}

void receiver_start(void) {
    // Initialize LoRa                  
    lora_init();
    lora_set_bandwidth  (250000);
    lora_set_coding_rate (5); 
    lora_set_frequency (865000000);
    lora_set_sync_word (0XF3);
    lora_set_spreading_factor (12);
    lora_set_tx_power (17);
    lora_enable_crc();
    lora_explicit_header_mode();         

    // Start tasks
    xTaskCreatePinnedToCore(lora_receive_task, "lora_rx_task", 4096, NULL, 24, NULL, 0);
    // xTaskCreatePinnedToCore(lora_receive_task, "lora_rx_task", 4096, NULL, 10, NULL, 0);
    //xTaskCreate(buzzer_control_task, "buzzer_ctl", 1024, NULL, 5, NULL);
    
}



/*
#include "lora_receive.h"
#include <stdio.h>
#include <string.h>

#define TAG "LORA_RECEIVE"
#define MAX_SENDERS 100
#define SUPPRESSION_WINDOW 5000  // ms

typedef struct {
    char mac[18];
    int64_t last_seen;   // timestamp in ms
} SenderEntry;

static SenderEntry sender_table[MAX_SENDERS];
static int sender_count = 0;
QueueHandle_t lora_rx_queue = NULL;

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

// ================= Panic System Flags =================
bool shouldBeep = false;

// ================= Buzzer Task =================
void buzzer_control_task(void *pvParameter) {
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << BUZZER_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);
    gpio_set_level(BUZZER_PIN, 0);

    while (1) {
        gpio_set_level(BUZZER_PIN, shouldBeep ? 1 : 0);
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

FlatOwner* find_owner_by_mac(const char *mac) {
    for (int i = 0; i < total_owners; i++) {
        if (strcasecmp(owners[i].mac, mac) == 0) {
            return &owners[i];
        }
    }
    return NULL; // not found
}

// static void lora_receive_task(void *param){
//     uint8_t buf[256];
//     int len;

//     lora_receive();
//     ESP_LOGI(TAG, "LoRa Receiver Task Started");
//     while (1) {
//         len = lora_receive_packet(buf, sizeof(buf));
        
//         //xQueueSend(lora_rx_queue, &buf, portMAX_DELAY);
//         if (len > 0) {
//             //buf[len] = 0; // Null-terminate for printing
//             if (len >= sizeof(buf)) 
//             len = sizeof(buf) - 1;
//             buf[len] = '\0'; // safe string termination
//             lcd_register_activity();
            
//             ESP_LOGI(TAG, "Received: %s (len=%d)", (char*)buf, len);

//             if (should_accept((char*)buf)) {
//                 FlatOwner msg={0};
//                 FlatOwner *owner = find_owner_by_mac((char*)buf);
//                 if (owner) {
//                     // shouldBeep = true;
//                     // i2c_lcd_clear();
//                     // i2c_lcd_set_cursor(0, 0);
//                     // i2c_lcd_send_string(owner->name);
//                     // i2c_lcd_set_cursor(1, 0);
//                     // i2c_lcd_send_string(owner->flat);

//                     strcpy(msg.name, owner->name);
//                     strcpy(msg.flat, owner->flat);
//                     shouldBeep = true;
//                     ESP_LOGI(TAG, "Owner Found: %s, Flat %s", owner->name, owner->flat);
//                     log_received_data(false, owner->name, owner->flat);

//                     char timestr[32]; 
//                     rtc_get_time_str(timestr, sizeof(timestr));
//                     ESP_LOGI(TAG, "Received at: %s", timestr);

//                 } else {
//                     strcpy(msg.name, "Unknown");
//                     strcpy(msg.flat, (char*)buf);

//                     ESP_LOGW(TAG, "MAC not in CSV: %s", (char*)buf);
//                 }
//                 xQueueOverwrite(lora_rx_queue,&msg);
//             // } else {
//             //     // just skip duplicates
//             }
//             //lora_receive();
//         }
//         vTaskDelay(pdMS_TO_TICKS(100)); // Small delay to yield CPU
//     }
// }

static void lora_receive_task(void *param)
{
    uint8_t buf[64];   // MAC string does NOT need 256 bytes
    int len;

    ESP_LOGI(TAG, "LoRa Receiver Task Started");

    lora_receive();  // enter RX mode ONCE

    while (1)
    {
        len = lora_receive_packet(buf, sizeof(buf) - 1);

        if (len > 0)
        {
            buf[len] = '\0';  // safe termination
            ESP_LOGI(TAG, "LoRa RX: %s", (char *)buf);

            if (!should_accept((char *)buf)) {
                continue;
            }

            FlatOwner msg = {0};
            FlatOwner *owner = find_owner_by_mac((char *)buf);

            if (owner) {
                strcpy(msg.name, owner->name);
                strcpy(msg.flat, owner->flat);
                shouldBeep = true;
                log_received_data(false, owner->name, owner->flat);
            } else {
                strcpy(msg.name, "Unknown");
                strcpy(msg.flat, (char *)buf);
            }
            lora_receive();
            // Send to STATE MACHINE
            xQueueOverwrite(lora_rx_queue, &msg);
        }

        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

void receiver_start(void) {

    lora_rx_queue = xQueueCreate(20, sizeof(FlatOwner));
    assert(lora_rx_queue);

    // Initialize LoRa                  
    lora_init();
    lora_set_bandwidth  (250000);
    lora_set_coding_rate (5); 
    lora_set_frequency (865000000);
    lora_set_sync_word (0XF3);
    lora_set_spreading_factor (12);
    lora_set_tx_power (17);
    lora_enable_crc();
    lora_explicit_header_mode();         

    // Start tasks
    //xTaskCreatePinnedToCore(lora_receive_task, "lora_rx_task", 4096, NULL, 24, NULL, 0);
    
    xTaskCreatePinnedToCore(lora_receive_task, "lora_rx_task", 4096, NULL, 10, NULL, 0);
    xTaskCreate(buzzer_control_task, "buzzer_ctl", 1024, NULL, 5, NULL);
}
*/