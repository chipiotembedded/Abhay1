// flash_csv.c - Embeds CSV file into firmware and loads to LittleFS at boot
#include "flash.h"
#include "esp_log.h"
#include "esp_littlefs.h"
#include "esp_spi_flash.h"
#include "esp_partition.h"
#include <string.h>

static const char *TAG = "FLASH_CSV";

// Default CSV content (name, flat, MAC)
// Modify this with your actual data
const char default_csv_content[] = 
    "Name,Flat,MAC\n"
    "John Doe,A101,44:1D:64:42:DB:40\n"
    "Jane Smith,A102,FC:B4:67:F4:83:E4\n"
    "Bob Johnson,A103,AA:BB:CC:DD:EE:FF\n";

// Partition label for storage (must match partition table)
#define STORAGE_PARTITION "storage"

// File paths
#define CSV_PATH "/littlefs/test.csv"
#define PASS_PATH "/littlefs/password.txt"
#define WIFI_PATH "/littlefs/wifi_credentials.txt"

// Default password (modify as needed)
const char default_password[] = "1234\n";

// Default WiFi credentials (modify as needed)
const char default_wifi[] = "YourSSID\nYourPassword\n";

esp_err_t flash_csv_init(void) {
    ESP_LOGI(TAG, "Initializing flash CSV...");
    
    // Check if file already exists
    FILE *f = fopen(CSV_PATH, "r");
    if (f) {
        ESP_LOGI(TAG, "CSV file already exists, skipping creation");
        fclose(f);
        return ESP_OK;
    }
    
    // Create CSV file
    f = fopen(CSV_PATH, "w");
    if (!f) {
        ESP_LOGE(TAG, "Failed to create CSV file");
        return ESP_FAIL;
    }
    
    size_t written = fwrite(default_csv_content, 1, strlen(default_csv_content), f);
    fclose(f);
    
    if (written != strlen(default_csv_content)) {
        ESP_LOGE(TAG, "Failed to write full CSV content");
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "CSV file created successfully with %d bytes", written);
    return ESP_OK;
}

esp_err_t flash_password_init(void) {
    ESP_LOGI(TAG, "Initializing password file...");
    
    FILE *f = fopen(PASS_PATH, "r");
    if (f) {
        ESP_LOGI(TAG, "Password file already exists, skipping creation");
        fclose(f);
        return ESP_OK;
    }
    
    f = fopen(PASS_PATH, "w");
    if (!f) {
        ESP_LOGE(TAG, "Failed to create password file");
        return ESP_FAIL;
    }
    
    size_t written = fwrite(default_password, 1, strlen(default_password), f);
    fclose(f);
    
    if (written != strlen(default_password)) {
        ESP_LOGE(TAG, "Failed to write full password content");
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "Password file created successfully");
    return ESP_OK;
}

esp_err_t flash_wifi_init(void) {
    ESP_LOGI(TAG, "Initializing WiFi credentials file...");
    
    FILE *f = fopen(WIFI_PATH, "r");
    if (f) {
        ESP_LOGI(TAG, "WiFi credentials file already exists, skipping creation");
        fclose(f);
        return ESP_OK;
    }
    
    f = fopen(WIFI_PATH, "w");
    if (!f) {
        ESP_LOGE(TAG, "Failed to create WiFi credentials file");
        return ESP_FAIL;
    }
    
    size_t written = fwrite(default_wifi, 1, strlen(default_wifi), f);
    fclose(f);
    
    if (written != strlen(default_wifi)) {
        ESP_LOGE(TAG, "Failed to write full WiFi credentials");
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "WiFi credentials file created successfully");
    return ESP_OK;
}

esp_err_t flash_all_defaults(void) {
    esp_err_t ret;
    
    ret = flash_csv_init();
    if (ret != ESP_OK) return ret;
    
    ret = flash_password_init();
    if (ret != ESP_OK) return ret;
    
    ret = flash_wifi_init();
    if (ret != ESP_OK) return ret;
    
    ESP_LOGI(TAG, "All default files created successfully");
    return ESP_OK;
}