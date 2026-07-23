#include "log_manager.h"

// static time_t file_creation_time;
static const char *TAG = "LOG_MANAGER";

/* Save original logger */
static FILE *log_file = NULL;
static vprintf_like_t original_vprintf = NULL;

/* Custom logger */

static int custom_logger(const char *fmt, va_list args)
{
    // Copy args because va_list can only be used once //
    va_list args_copy;
    va_copy(args_copy, args);

    //int ret = 0;
    // Print to UART normally //
    if (original_vprintf) {
        original_vprintf(fmt, args);
    }

    // Format log into buffer //
    char buffer[256];
    vsnprintf(buffer, sizeof(buffer), fmt, args_copy);
    va_end(args_copy);

    //  Write to SPIFFS file //
    FILE *f = fopen(LOG_FILE_PATH, "a");
    if (f) {
        time_t now;
        time(&now);

        struct tm timeinfo;
        localtime_r(&now, &timeinfo);

        char timestamp[32];
        strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", &timeinfo);

        fprintf(f, "[%s] %s", timestamp, buffer);
        fclose(f);
    }

    return 0;

}

void log_manager_init(void) {
    FILE *f = fopen(LOG_FILE_PATH, "w");
    if (f) {
        fclose(f);
        printf("Log file created/overwritten\n");
    }

    // Add small delay to let system stabilize
    vTaskDelay(pdMS_TO_TICKS(100));
    
    original_vprintf = esp_log_set_vprintf(custom_logger);
    printf("CUSTOM LOGGER TRIGGERED\n");
}

/*
void log_manager_init(void)
{
    //  OVERWRITE FILE EVERY BOOT //
    
    // FILE *f = fopen(LOG_FILE_PATH, "w"); 
    // if (f) {
    //     fclose(f);
    //     ESP_LOGI(TAG, "Log file created/overwritten");
    // }
    log_file = fopen(LOG_FILE_PATH, "w"); 
    if (!log_file) {
        printf("Failed to open log file\n");
        return;
    }
    printf("Opened log file\n");

    // Write initial boot marker
    time_t now;
    time(&now);
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);
    
    char timestamp[32];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", &timeinfo);

    fprintf(log_file, "[%s] === System Started ===\n", timestamp);
    fflush(log_file);

    // Hook logger AFTER file is cleared //
    //original_vprintf = esp_log_set_vprintf(custom_logger);
    esp_log_set_vprintf(custom_logger);
    printf("CUSTOM LOGGER TRIGGERED\n");
}
*/

/*
int custom_logger(const char *fmt, va_list args) {
    // Print to serial first (so ESP_LOGI still shows on console)
    int result = vprintf(fmt, args);
    
    // Also write to file if file is open
    if (log_file) {
        va_list args_copy;
        va_copy(args_copy, args);
        
        char buffer[256];
        vsnprintf(buffer, sizeof(buffer), fmt, args_copy);
        va_end(args_copy);
        
        // Write to file
        fprintf(log_file, "%s", buffer);
        fflush(log_file);
    }
    
    return result;
}
*/

/* ---------------- COPY FILE ---------------- */
esp_err_t copy_log_to_usb(void)
{
    /* Ensure all logs are written before copying */
    // fflush(NULL);
    if (log_file) {
        fflush(log_file);
    }

    FILE *src = fopen(LOG_FILE_PATH, "r");
    if (!src) {
        ESP_LOGE(TAG, "Cannot open %s", LOG_FILE_PATH);
        return ESP_FAIL;
    }

    fseek(src, 0, SEEK_END);
    long size = ftell(src);
    rewind(src);
    ESP_LOGI(TAG, "LittleFS log file size: %ld bytes", size);

    FILE *dst = fopen(LOG_USB_PATH, "w");
    if (!dst) {
        ESP_LOGE(TAG, "Cannot open %s", LOG_USB_PATH);
        fclose(src);
        return ESP_FAIL;
    }

    char buf[BUF_SIZE];
    size_t n;

    while ((n = fread(buf, 1, sizeof(buf), src)) > 0) {
        fwrite(buf, 1, n, dst);
    }

    fclose(src);
    fclose(dst);

    ESP_LOGI(TAG, "log.txt copied to USB successfully");
    return ESP_OK;
}