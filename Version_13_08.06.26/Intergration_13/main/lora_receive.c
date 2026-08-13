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

static void lora_receive_task(void *param)
{
    uint8_t buf[100];
    int len;

    lora_receive();
    ESP_LOGI(TAG, "LoRa Receiver Task Started");

    while (1){
        static int64_t last_log = 0;
        int64_t now = esp_timer_get_time() / 1000;
        if (now - last_log > 5000)   // every 5 seconds
        {
            last_log = now;
            ESP_LOGI(TAG,
                    "REG_VERSION=0x%02X OP_MODE=0x%02X IRQ=0x%02X",
                    lora_read_reg(REG_VERSION),
                    lora_read_reg(REG_OP_MODE),
                    lora_read_reg(REG_IRQ_FLAGS));
        }

        len = lora_receive_packet(buf, sizeof(buf) - 1);
        if (len > 0)
        {
            // Ensure safe null termination
            if (len >= sizeof(buf))
                len = sizeof(buf) - 1;

            buf[len] = '\0';
            lcd_register_activity();
            ESP_LOGI(TAG, "Received raw: %s (len=%d)", (char *)buf, len);

            // ---------- Safe Packet Parsing ----------
            char temp[100];
            snprintf(temp, sizeof(temp), "%s", (char *)buf);

            // Extract sender before '|'
            char *sep = strchr(temp, '|');
            if (sep != NULL){
                *sep = '\0';  // terminate sender string
            }
            char *sender = temp;

            // Validate sender
            if (sender == NULL || sender[0] == '\0'){
                ESP_LOGW(TAG, "Invalid packet format");
                lora_receive();
                continue;
            }
            ESP_LOGI(TAG, "Parsed sender: %s", sender);

            // ---------- Duplicate Suppression ----------
            if (!should_accept(sender)){
                lora_receive();
                continue;
            }
            //vTaskDelay(pdMS_TO_TICKS(100));
            // ---------- Owner Lookup ----------
            FlatOwner *owner = find_owner_by_mac(sender);
            if (owner){
                shouldBeep = true;
                
                lcd_display_text(0, 0, owner->name, true);
                lcd_display_text(1, 0, owner->flat, false);
                //vTaskDelay(pdMS_TO_TICKS(10));


                ESP_LOGI(TAG, "Owner Found: %s, Flat %s",
                         owner->name, owner->flat);

                log_received_data(false, owner->name, owner->flat);

                char timestr[32];
                rtc_get_time_string(timestr, sizeof(timestr));

                ESP_LOGI(TAG, "Received at: %s", timestr);
            }
            else{
                lcd_display_text(0, 0, "Unknown MAC", true);
                lcd_display_text(1, 0, sender, false);
                ESP_LOGW(TAG, "MAC not in CSV: %s", sender);
            }
            lora_receive();
            ESP_LOGI(TAG,
            "After lora_receive(): OP_MODE=0x%02X",
            lora_read_reg(REG_OP_MODE));
        }
        vTaskDelay(pdMS_TO_TICKS(10));
        esp_task_wdt_reset();
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
    lora_enable_crc();
    lora_explicit_header_mode();  
    
    TaskHandle_t lora_handle = NULL;

    // Start tasks
    //xTaskCreate(lora_receive_task, "lora_rx_task", 8196, NULL, 10, NULL);
    xTaskCreatePinnedToCore(lora_receive_task, "lora_rx_task", 8192, NULL, 10, &lora_handle, 0);
    //xTaskCreatePinnedToCore(buzzer_control_task, "buzzer_ctl", 4096, NULL, 5, NULL, 0);
    xTaskCreate(buzzer_control_task, "buzzer_ctl", 4096, NULL, 5, NULL);
    
    // if (lora_handle) esp_task_wdt_add(lora_handle);
    // if (buzzer_handle) esp_task_wdt_add(buzzer_handle);
    if (lora_handle) {
        esp_err_t wdt_err = esp_task_wdt_add(lora_handle);
        if (wdt_err == ESP_OK) {
            ESP_LOGI(TAG, "lora_rx_task registered with Task Watchdog");
        } else {
            ESP_LOGW(TAG, "Failed to register lora_rx_task with Task Watchdog: %s",
                     esp_err_to_name(wdt_err));
        }
    }
}
