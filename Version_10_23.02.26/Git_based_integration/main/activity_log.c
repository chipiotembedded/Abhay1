#include "activity_log.h"

int total_logs = 0;
int current_log_index = 0;

void initialize_activity_log() {
    // esp_vfs_littlefs_conf_t conf = {
    //     .base_path = "/littlefs",
    //     .partition_label = "storage",
    //     .format_if_mount_failed = true,
    //     .dont_mount = false,
    // };

    // esp_err_t ret = esp_vfs_littlefs_register(&conf);
    // if (ret != ESP_OK) {
    //     ESP_LOGE(ACTIVITY_LOG_TAG, "LittleFS mount failed");
    //     return;
    // }

    ESP_LOGI(ACTIVITY_LOG_TAG, "LittleFS mounted");

    total_logs = get_total_logs();
}

void log_received_data(bool buttonPressed, const char *OwnerName, const char *FlatNumber) {
    FILE *f = fopen(ACTIVITY_FILE_PATH, "a");   // append binary
    if (!f) {
        ESP_LOGE(ACTIVITY_LOG_TAG, "Failed to open log file");
        return;
    }

    log_entry entry;
    entry.buttonPressed = buttonPressed;

    strncpy(entry.OwnerName, OwnerName, sizeof(entry.OwnerName)-1);
    entry.OwnerName[sizeof(entry.OwnerName)-1] = '\0';

    strncpy(entry.FlatNumber, FlatNumber, sizeof(entry.FlatNumber)-1);
    entry.FlatNumber[sizeof(entry.FlatNumber)-1] = '\0';

    rtc_get_time_string(entry.timestamp, sizeof(entry.timestamp));

    fwrite(&entry, sizeof(log_entry), 1, f);
    fclose(f);

    total_logs++;

    // Optional circular buffer handling
    if (total_logs > MAX_LOGS) {
        trim_old_logs();
        total_logs = MAX_LOGS;
    }

    ESP_LOGI(ACTIVITY_LOG_TAG, "Log stored. Total: %d", total_logs);
}


int get_total_logs() {
    FILE *f = fopen(ACTIVITY_FILE_PATH, "r");
    if (!f) return 0;

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fclose(f);

    return size / sizeof(log_entry);
}

int read_log_entry(int index, log_entry *entry) {
    FILE *f = fopen(ACTIVITY_FILE_PATH, "r");
    if (!f) return 0;

    fseek(f, index * sizeof(log_entry), SEEK_SET);

    size_t r = fread(entry, sizeof(log_entry), 1, f);
    fclose(f);

    return (r == 1);
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

void trim_old_logs()
{
    FILE *f = fopen(ACTIVITY_FILE_PATH, "r");
    if (!f) return;

    log_entry buffer[MAX_LOGS];

    fseek(f, sizeof(log_entry), SEEK_SET);
    fread(buffer, sizeof(log_entry), MAX_LOGS - 1, f);
    fclose(f);

    f = fopen(ACTIVITY_FILE_PATH, "w");
    fwrite(buffer, sizeof(log_entry), MAX_LOGS - 1, f);
    fclose(f);
}

// Add this temporary debug function to activity_log.c
void debug_print_all_logs() {
    //log_entry entry;
    ESP_LOGI(ACTIVITY_LOG_TAG, "--- DUMPING ALL LOGS ---");
    ESP_LOGI(ACTIVITY_LOG_TAG, "Total logs: %d", total_logs);
    
    ESP_LOGI(ACTIVITY_LOG_TAG, "Entered Activity log");
}

// Send activity_log txt file to usb //
esp_err_t copy_activity_to_usb(void)
{
    //LCD string
    // i2c_lcd_clear();
    // i2c_lcd_set_cursor(2,0);
    // i2c_lcd_send_string("Sending Act to USB"); 

    FILE *src = fopen(ACTIVITY_FILE_PATH, "r");
    if (!src) {
        ESP_LOGE("ACTIVITY_COPY", "Failed to open source file");
        return ESP_FAIL;
    }

    FILE *dst = fopen(ACTIVITY_USB_FILE_PATH, "w");
    if (!dst) {
        ESP_LOGE("ACTIVITY_COPY", "Failed to open USB file");
        fclose(src);
        return ESP_FAIL;
    }

    // uint8_t buffer[COPY_BUFFER_SIZE];
    // size_t bytes_read;
    // while ((bytes_read = fread(buffer, 1, sizeof(buffer), src)) > 0) {
    //     fwrite(buffer, 1, bytes_read, dst);
    // }

    // char line[128];

    // while (fgets(line, sizeof(line), src)) {
    //     fprintf(dst, "%s", line);  // write exactly as is
    //     printf("LOG: %s", line);
    // }
    
    log_entry entry;

    while (fread(&entry, sizeof(log_entry), 1, src) == 1) {
        fprintf(dst, "%s,%s,%s\n",
                entry.OwnerName,
                entry.FlatNumber,
                entry.timestamp);
    }
    fclose(src);
    fclose(dst);

    ESP_LOGI("ACTIVITY_COPY", "Activity log copied to USB successfully");

    return ESP_OK;
}