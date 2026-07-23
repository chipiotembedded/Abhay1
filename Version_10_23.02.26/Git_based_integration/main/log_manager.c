#include "log_manager.h"

// static time_t file_creation_time;
static const char *TAG = "LOG_MANAGER";

/* Save original logger */
static FILE *log_file = NULL;
static vprintf_like_t original_vprintf = NULL;

/* Custom logger */
static int custom_logger(const char *fmt, va_list args)
{
    /* Copy args because va_list can only be used once */
    va_list args_copy;
    va_copy(args_copy, args);

    //int ret = 0;
    /* Print to UART normally */
    if (original_vprintf) {
        original_vprintf(fmt, args);
    }

    /* Format log into buffer */
    char buffer[256];
    vsnprintf(buffer, sizeof(buffer), fmt, args_copy);
    va_end(args_copy);

    // /* Write to SPIFFS file */
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

    // if (log_file) {
    //     char buffer[256];
    //     vsnprintf(buffer, sizeof(buffer), fmt, args_copy);

    //     time_t now;
    //     time(&now);

    //     struct tm timeinfo;
    //     localtime_r(&now, &timeinfo);

    //     char timestamp[32];
    //     strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", &timeinfo);

    //     fprintf(log_file, "[%s] %s", timestamp, buffer);
    //     fflush(log_file);
    // }

    // va_end(args_copy);
    // // printf("CUSTOM LOGGER TRIGGERED\n");
    // return ret;
}

void log_manager_init(void)
{
    /* 🔥 OVERWRITE FILE EVERY BOOT */
    // log_file = fopen(LOG_FILE_PATH, "w");   // "w" clears file
    FILE *f = fopen(LOG_FILE_PATH, "w"); 
    if (f) {
        fclose(f);
        ESP_LOGI(TAG, "Log file created/overwritten");
    }

    //file_creation_time = time(NULL);

    // if (!log_file) {
    //     printf("Failed to open log file\n");
    //     return;
    // }
    printf("Opened log file\n");
    // fprintf(log_file, "TEST LINE\n");
    // fflush(log_file);

    /* Hook logger AFTER file is cleared */
    original_vprintf = esp_log_set_vprintf(custom_logger);
    printf("CUSTOM LOGGER TRIGGERED\n");
}

/* ---------------- COPY FILE ---------------- */
esp_err_t copy_log_to_usb(void)
{
    //lcd string
    // i2c_lcd_clear();
    // i2c_lcd_set_cursor(1,0);
    // i2c_lcd_send_string("Sending to USB"); 

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