/*
//Working code
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_vfs.h"
#include "esp_littlefs.h"

#include "lora.h"
#include "lcd_i2c.h"
#include "flash_csv.h"
#include "lora_receive.h"
#include "activity_log.h"
#include "system_states.h"
#include "rtc.h"

#define TAG "LORA_RECEIVE"

// ================= LoRa Parameters =================
// #define LORA_FREQUENCY       865000000   // 865 MHz (India)
// #define LORA_TX_POWER        17
// #define LORA_SPREADING_FACTOR 12
// #define LORA_BANDWIDTH       250000      // 250 kHz
// #define LORA_CODING_RATE     7           // 4/7
// #define SYNC_WORD            0x12
// #define RECEIVE_TIMEOUT_MS   10000
// #define RSSI_THRESHOLD       -120

#define LORA_FREQUENCY       865000000   // 865 MHz (India)
#define LORA_TX_POWER        17
#define LORA_SPREADING_FACTOR 10
#define LORA_BANDWIDTH       125000      // 250 kHz
#define LORA_CODING_RATE     5           // 4/7
#define SYNC_WORD            0x12
#define RECEIVE_TIMEOUT_MS   10000
#define RSSI_THRESHOLD       -120

// #define LORA_FREQUENCY       865000000   // 865 MHz (India)
// #define LORA_TX_POWER        20
// #define LORA_SPREADING_FACTOR 12
// #define LORA_BANDWIDTH       125000      // 250 kHz
// #define LORA_CODING_RATE     8           // 4/7
// #define SYNC_WORD            0x12
// #define RECEIVE_TIMEOUT_MS   10000
// #define RSSI_THRESHOLD       -120

// ================= Message Format =================
typedef struct {
    uint8_t msg_type;      // DATA=0x01, ACK=0x02
    uint8_t src_mac[6];
    uint8_t dst_mac[6];
    uint8_t hop_count;
    uint8_t msg_id;
    char payload[32];      // MAC string of sender (for CSV lookup)
} __attribute__((packed)) lora_message_t;

#define DATA_MSG 0x01
#define ACK_MSG  0x02

// ================= Receiver State =================
static uint8_t receiver_mac[6];

// ================= Stats =================
typedef struct {
    uint32_t total_messages;
    uint32_t valid_messages;
    uint32_t crc_errors;
    uint32_t size_errors;
    uint32_t timeout_errors;
    uint32_t low_rssi_discards;
    int32_t best_rssi;
    int32_t worst_rssi;
    float best_snr;
    float worst_snr;
} receiver_stats_t;

static receiver_stats_t stats = {0,0,0,0,0,0,-200,0,-50.0,50.0};

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

// ================= Button Task =================
static void button_task(void *pvParameter) {
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << Silence_BUTTON),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);

    while (1) {
        if (gpio_get_level(Silence_BUTTON) == 0) {
            shouldBeep = false;
            gpio_set_level(BUZZER_PIN, 0);
            ESP_LOGI(TAG, "Buzzer silenced");

            // i2c_lcd_clear();
            // i2c_lcd_set_cursor(0, 0);
            // i2c_lcd_send_string("Buzzer Silenced");

            while (gpio_get_level(Silence_BUTTON) == 0) {
                vTaskDelay(pdMS_TO_TICKS(10));
            }
        }
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

// ================= ACK =================
void send_ack(lora_message_t *msg) {
    lora_message_t ack_msg;
    ack_msg.msg_type = ACK_MSG;
    memcpy(ack_msg.src_mac, receiver_mac, 6);
    memcpy(ack_msg.dst_mac, msg->src_mac, 6);
    ack_msg.hop_count = 0;
    ack_msg.msg_id = msg->msg_id;
    snprintf(ack_msg.payload, sizeof(ack_msg.payload), "ACK_%d", msg->msg_id);

    vTaskDelay(pdMS_TO_TICKS(LORA_SPREADING_FACTOR * 100));
    // lora_send_packet((uint8_t*)&ack_msg, sizeof(ack_msg));
    // ESP_LOGI(TAG, "ACK sent for msg %d", msg->msg_id);
    lora_receive();
}

// ================= Process Incoming Packet =================
void process_received(lora_message_t *msg, int rssi, float snr) {
    stats.valid_messages++;

    // Update stats
    if (rssi > stats.best_rssi) stats.best_rssi = rssi;
    if (rssi < stats.worst_rssi) stats.worst_rssi = rssi;
    if (snr > stats.best_snr) stats.best_snr = snr;
    if (snr < stats.worst_snr) stats.worst_snr = snr;

    ESP_LOGI(TAG, "Packet: ID=%d Type=%s RSSI=%d SNR=%.2f",
             msg->msg_id,
             msg->msg_type == DATA_MSG ? "DATA" : "ACK",
             rssi, snr);

    // Only process DATA messages
    if (msg->msg_type == DATA_MSG) {
        FlatOwner *owner = find_owner_by_mac(msg->payload);
        if (owner) {
            shouldBeep = true;
            i2c_lcd_clear();
            i2c_lcd_set_cursor(0, 0);
            i2c_lcd_send_string(owner->name);
            i2c_lcd_set_cursor(1, 0);
            i2c_lcd_send_string(owner->flat);

            ESP_LOGI(TAG, "Owner Found: %s, Flat %s", owner->name, owner->flat);
            log_received_data(false, owner->name, owner->flat);

            //date and time to log
            char timestr[32];
            rtc_get_time_str(timestr, sizeof(timestr));
            ESP_LOGI(TAG, "Received at: %s", timestr);

        } else {
            i2c_lcd_clear();
            i2c_lcd_set_cursor(0, 0);
            i2c_lcd_send_string("Unknown MAC");
            i2c_lcd_set_cursor(1, 0);
            i2c_lcd_send_string(msg->payload);

            ESP_LOGW(TAG, "MAC not in CSV: %s", msg->payload);
        }
        // Determine signal quality
        const char* signal_quality;
        if (rssi > -60) signal_quality = "EXCELLENT";
        else if (rssi > -80) signal_quality = "GOOD";
        else if (rssi > -100) signal_quality = "FAIR";
        else if (rssi > -120) signal_quality = "WEAK";
        else signal_quality = "VERY WEAK";

        ESP_LOGI(TAG, "");
        ESP_LOGI(TAG, "MESSAGE RECEIVED SUCCESSFULLY!");
        ESP_LOGI(TAG, "============================================");
        ESP_LOGI(TAG, "Message ID:   %d", msg->msg_id);
        ESP_LOGI(TAG, "Message Type: %s", msg->msg_type == DATA_MSG ? "DATA" : "ACK");
        ESP_LOGI(TAG, "Source MAC:   %02X:%02X:%02X:%02X:%02X:%02X", 
                msg->src_mac[0], msg->src_mac[1], msg->src_mac[2], 
                msg->src_mac[3], msg->src_mac[4], msg->src_mac[5]);
        ESP_LOGI(TAG, "Hop Count:    %d", msg->hop_count);
        ESP_LOGI(TAG, "Route:        %s", msg->hop_count == 0 ? "DIRECT" : "VIA REPEATER");
        ESP_LOGI(TAG, "Payload:      %s", msg->payload);
        ESP_LOGI(TAG, "");
        ESP_LOGI(TAG, "SIGNAL INFORMATION");
        ESP_LOGI(TAG, "============================================");
        ESP_LOGI(TAG, "RSSI:           %d dBm", rssi);
        ESP_LOGI(TAG, "SNR:            %.2f dB", snr);
        ESP_LOGI(TAG, "Signal Quality: %s", signal_quality);
        ESP_LOGI(TAG, "============================================");

        send_ack(msg);
    }
}

// ================= LoRa Receive Task =================
static void lora_receive_task(void *param) {
    lora_receive();
    while (1) {
        if (lora_received()) {
            uint8_t buf[256];
            int len = lora_receive_packet(buf, sizeof(buf));
            int rssi = lora_packet_rssi();
            float snr = lora_packet_snr();
            stats.total_messages++;

            if (rssi < RSSI_THRESHOLD) {  
                stats.low_rssi_discards++;
                ESP_LOGW(TAG, "Weak RSSI %d, discarded", rssi);
                lora_receive();
                continue;
            }
            if (len == sizeof(lora_message_t)) {
                process_received((lora_message_t*)buf, rssi, snr);
            } else if (len == -1) {
                stats.crc_errors++;
                ESP_LOGW(TAG, "CRC error (RSSI=%d)", rssi);
                process_received((lora_message_t*)buf, rssi, snr);
            } else if (len > 0) {
                stats.size_errors++;
                ESP_LOGW(TAG, "Size error: expected %zu, got %d",
                         sizeof(lora_message_t), len);
            }
            lora_receive();
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

// ================= Init =================
void receiver_start(void) {
    // Get MAC
    if (esp_wifi_get_mac(WIFI_IF_STA, receiver_mac) != ESP_OK) {
        memcpy(receiver_mac, (uint8_t[]){0x11,0x22,0x33,0x44,0x55,0x01}, 6);
    }

    // Init LoRa;
    lora_init();
    lora_set_frequency(LORA_FREQUENCY);
    lora_set_tx_power(LORA_TX_POWER);
    lora_set_spreading_factor(LORA_SPREADING_FACTOR);
    lora_set_bandwidth(LORA_BANDWIDTH);
    lora_set_coding_rate(LORA_CODING_RATE);
    lora_set_sync_word(SYNC_WORD);
    lora_enable_crc();
    lora_explicit_header_mode();

    // Start tasks
    xTaskCreate(lora_receive_task, "lora_rx_task", 4096, NULL, 5, NULL);
    xTaskCreate(buzzer_control_task, "buzzer_ctl", 2048, NULL, 5, NULL);
    xTaskCreate(button_task, "button_task", 2048, NULL, 6, NULL);

    ESP_LOGI(TAG, "Receiver started (SF%d, BW%d)", 
             LORA_SPREADING_FACTOR, LORA_BANDWIDTH/1000);
}
*/



#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_vfs.h"
#include "esp_littlefs.h"
#include "esp_timer.h"   // for esp_timer_get_time()

#include "lora.h"
#include "lcd_i2c.h"
#include "flash_csv.h"
#include "lora_receive.h"
#include "activity_log.h"
#include "system_states.h"
#include "button_handler.h"
#include "rtc.h"

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

static void lora_receive_task(void *param){
    uint8_t buf[256];
    int len;

    lora_receive();
    ESP_LOGI(TAG, "LoRa Receiver Task Started");
    while (1) {
        len = lora_receive_packet(buf, sizeof(buf));
        if (len > 0) {
            //buf[len] = 0; // Null-terminate for printing
            if (len >= sizeof(buf)) len = sizeof(buf) - 1;
            buf[len] = '\0'; // safe string termination

            ESP_LOGI(TAG, "Received: %s (len=%d)", (char*)buf, len);

            if (should_accept((char*)buf)) {
                FlatOwner *owner = find_owner_by_mac((char*)buf);
                if (owner) {
                    shouldBeep = true;
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
    xTaskCreate(buzzer_control_task, "buzzer_ctl", 4096, NULL, 5, NULL);
    
}