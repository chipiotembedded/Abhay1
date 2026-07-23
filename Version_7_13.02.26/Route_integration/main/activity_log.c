#include "activity_log.h"

SemaphoreHandle_t nvs_mutex = NULL;
int total_logs = 0;
int current_log_index = 0;

void initialize_activity_log() {
    // Create mutex for NVS if not already created
    if (nvs_mutex == NULL) {
        nvs_mutex = xSemaphoreCreateMutex();
    }
    
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    // Verify NVS
    nvs_stats_t nvs_stats;
    nvs_get_stats(NULL, &nvs_stats);
    ESP_LOGI("NVS", "Used=%d, Free=%d", nvs_stats.used_entries, nvs_stats.free_entries);
    
    // Initialize log count
    if (xSemaphoreTake(nvs_mutex, portMAX_DELAY) == pdTRUE) {
        total_logs = get_total_logs();
        xSemaphoreGive(nvs_mutex);
    }
    
    ESP_LOGI(ACTIVITY_LOG_TAG, "Activity log initialized with %d logs", total_logs);
}

void log_received_data(bool buttonPressed, const char *OwnerName, const char *FlatNumber) {
    if (xSemaphoreTake(nvs_mutex, portMAX_DELAY) == pdTRUE) {
        nvs_handle_t nvs_handle;
        esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
        if (err != ESP_OK) {
            ESP_LOGE(ACTIVITY_LOG_TAG, "Error opening NVS: %s", esp_err_to_name(err));
            xSemaphoreGive(nvs_mutex);
            return;
        }

        uint32_t log_index;
        err = nvs_get_u32(nvs_handle, "log_index", &log_index);
        if (err != ESP_OK) log_index = 0;
        if (log_index >= MAX_LOGS) log_index = 0;

        log_entry entry;
        entry.buttonPressed = buttonPressed;
        strncpy(entry.OwnerName, OwnerName, sizeof(entry.OwnerName) - 1);
        entry.OwnerName[sizeof(entry.OwnerName) - 1] = '\0';
        strncpy(entry.FlatNumber, FlatNumber, sizeof(entry.FlatNumber) - 1);
        entry.FlatNumber[sizeof(entry.FlatNumber) - 1] = '\0';
        //entry.timestamp = time(NULL);
        rtc_get_time_str(entry.timestamp, sizeof(entry.timestamp));

        char log_key[20];
        snprintf(log_key, sizeof(log_key), "log_%lu", (unsigned long)log_index);
        err = nvs_set_blob(nvs_handle, log_key, &entry, sizeof(log_entry));

        if (err == ESP_OK) {
            log_index++;
            nvs_set_u32(nvs_handle, "log_index", log_index);
            nvs_commit(nvs_handle);
            //total_logs = (log_index > MAX_LOGS) ? MAX_LOGS : log_index;
            if (log_index < MAX_LOGS) {
                total_logs = log_index;
            } else {
                total_logs = MAX_LOGS;   // once wrapped, always full
            }
            ESP_LOGI(ACTIVITY_LOG_TAG, "Stored log %lu, total: %d", log_index - 1, total_logs);
        } else {
            ESP_LOGE(ACTIVITY_LOG_TAG, "Failed to store log: %s", esp_err_to_name(err));
        }

        nvs_close(nvs_handle);
        xSemaphoreGive(nvs_mutex);
    }
}


int get_total_logs() {
    nvs_handle_t nvs_handle;
    uint32_t log_index = 0;
    if (nvs_open(NVS_NAMESPACE, NVS_READONLY, &nvs_handle) == ESP_OK) {
        nvs_get_u32(nvs_handle, "log_index", &log_index);
        nvs_close(nvs_handle);
    }
    //return (log_index > MAX_LOGS) ? MAX_LOGS : log_index;
    return (log_index < MAX_LOGS) ? log_index : MAX_LOGS;

}

int read_log_entry(int index, log_entry *entry) {
    if (index >= total_logs) {
        ESP_LOGE(ACTIVITY_LOG_TAG, "Index %d out of range (total: %d)", index, total_logs);
        return 0;
    }
    
    // Calculate actual storage index (circular buffer)
    uint32_t actual_index = index;
    uint32_t log_index;
    nvs_handle_t nvs_handle;
    
    if (nvs_open(NVS_NAMESPACE, NVS_READONLY, &nvs_handle) != ESP_OK) {
        ESP_LOGE(ACTIVITY_LOG_TAG, "Failed to open NVS");
        return 0;
    }
    
    // Get current log index
    if (nvs_get_u32(nvs_handle, "log_index", &log_index) != ESP_OK) {
        nvs_close(nvs_handle);
        return 0;
    }
    
    // Handle circular buffer indexing
    if (log_index > MAX_LOGS) {
        actual_index = (log_index - MAX_LOGS + index) % MAX_LOGS;
    }
    
    char log_key[20];
    snprintf(log_key, sizeof(log_key), "log_%lu", (unsigned long)actual_index);

    size_t len = sizeof(log_entry);
    esp_err_t err = nvs_get_blob(nvs_handle, log_key, entry, &len);
    nvs_close(nvs_handle);

    if (err != ESP_OK) {
        ESP_LOGE(ACTIVITY_LOG_TAG, "Failed to read %s: %s", log_key, esp_err_to_name(err));
        return 0;
    }
    
    if (len != sizeof(log_entry)) {
        ESP_LOGE(ACTIVITY_LOG_TAG, "Invalid blob size for %s: %d", log_key, len);
        return 0;
    }
    
    return 1;
}

void display_log_entry(int index) {
    log_entry entry;

    // Reverse the index: latest first
    int actual_index = (total_logs - 1) - index;

    if (read_log_entry(actual_index, &entry)) {

        i2c_lcd_set_cursor(0, 0);
        i2c_lcd_send_string(entry.OwnerName);

        i2c_lcd_set_cursor(1, 0);
        i2c_lcd_send_string(entry.FlatNumber);

        i2c_lcd_set_cursor(2, 0);
        i2c_lcd_send_string(entry.timestamp);

        char index_str[16];
        snprintf(index_str, sizeof(index_str), "%d/%d", current_log_index  + 1, total_logs);
        i2c_lcd_set_cursor(3, 4);
        i2c_lcd_send_string("  ");  // 16 spaces
        i2c_lcd_set_cursor(3, 0);
        i2c_lcd_send_string(index_str);
    }
}

// Add this temporary debug function to activity_log.c
void debug_print_all_logs() {
    //log_entry entry;
    ESP_LOGI(ACTIVITY_LOG_TAG, "--- DUMPING ALL LOGS ---");
    ESP_LOGI(ACTIVITY_LOG_TAG, "Total logs: %d", total_logs);
    
    ESP_LOGI(ACTIVITY_LOG_TAG, "Entered Activity log");
}