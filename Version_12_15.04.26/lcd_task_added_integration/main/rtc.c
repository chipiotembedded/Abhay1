// #include "rtc.h"
// #include "nvs_flash.h"
// #include "nvs.h"
// #include "esp_log.h"

// static const char *TAG = "RTC";

// // Global time variable
// rtc_time_t current_time = {25, 9, 8, 12, 0, 0};

// // Days in each month
// static const int days_in_month[] = {31,28,31,30,31,30,31,31,30,31,30,31};

// bool is_leap_year(int year) {
//     return ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0));
// }

// void increment_time() {
//     current_time.sec++;
//     if (current_time.sec >= 60) {
//         current_time.sec = 0;
//         current_time.min++;
//     }
//     if (current_time.min >= 60) {
//         current_time.min = 0;
//         current_time.hours++;
//     }
//     if (current_time.hours >= 24) {
//         current_time.hours = 0;
//         current_time.day++;
//     }

//     int dim = days_in_month[current_time.month - 1];
//     if (current_time.month == 2 && is_leap_year(current_time.year)) {
//         dim = 29;
//     }

//     if (current_time.day > dim) {
//         current_time.day = 1;
//         current_time.month++;
//     }
//     if (current_time.month > 12) {
//         current_time.month = 1;
//         current_time.year++;
//     }
// }

// void save_time_on_nvs(void) {
//     nvs_handle_t handle;
//     if (nvs_open("storage", NVS_READWRITE, &handle) == ESP_OK) {
//         nvs_set_blob(handle, "rtc_time", &current_time, sizeof(current_time));
//         nvs_commit(handle);
//         nvs_close(handle);
//         //ESP_LOGI(TAG, "RTC time saved to NVS");
//     }
// }

// void load_time_from_nvs(void) {
//     nvs_handle_t handle;
//     size_t required_size = sizeof(current_time);
//     if (nvs_open("storage", NVS_READWRITE, &handle) == ESP_OK) {
//         if (nvs_get_blob(handle, "rtc_time", &current_time, &required_size) == ESP_OK) {
//             ESP_LOGI(TAG, "RTC time restored from NVS");
//         }
//         nvs_close(handle);
//     }
// }

// void rtc_task(void *pvParameters) {
//     while (1) {
//         increment_time();
//         save_time_on_nvs();
//         vTaskDelay(pdMS_TO_TICKS(1000)); // 1 second tick
//     }
// }

// void app_rtc_init(void) {
//     load_time_from_nvs();
//     xTaskCreate(rtc_task, "rtc_task", 2048, NULL, 5, NULL);
// }

// void rtc_get_time_str(char *buffer, int len) {
//     snprintf(buffer, len, "%02d-%02d-%02d %02d:%02d",
//             //  current_time.year, current_time.month, current_time.day,
//             //  current_time.hours, current_time.min, current_time.sec);
//             current_time.day,current_time.month, current_time.year,
//             current_time.hours, current_time.min);
// }


#include "rtc.h"
#include "sdkconfig.h"
#include "esp_sntp.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>

static const char *TAG = "RTC_DRIVER";

#define RTC_MIN_VALID_YEAR      2023
#define RTC_SYNC_TIMEOUT_MS     45000
#define RTC_SYNC_POLL_MS        500
#define RTC_SYNC_LOG_MS         2000
#define RTC_SYNC_LOG_COUNT      ((RTC_SYNC_TIMEOUT_MS + RTC_SYNC_LOG_MS - 1) / RTC_SYNC_LOG_MS)

static volatile bool rtc_time_synced = false;

// #define RTC_SYNC_RETRY_COUNT     5
// #define RTC_SYNC_RETRY_DELAY_MS  2000


// Internal Functions            //


static void rtc_time_sync_notification_cb(struct timeval *tv)
{
    rtc_time_synced = true;
    ESP_LOGI(TAG, "SNTP time sync callback received");
}

static void rtc_initialize_sntp(void)
{
    ESP_LOGI(TAG, "Initializing SNTP");

    //esp_sntp_stop();  // Add this line to stop any existing SNTP

    // Stop first if already running to avoid assert inside IDF
    if (esp_sntp_enabled()) {
        esp_sntp_stop();
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    rtc_time_synced = false;
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "pool.ntp.org");
#if CONFIG_LWIP_SNTP_MAX_SERVERS > 1
    esp_sntp_setservername(1, "time.google.com");
#endif
#if CONFIG_LWIP_SNTP_MAX_SERVERS > 2
    esp_sntp_setservername(2, "time.cloudflare.com");
#endif
#if CONFIG_LWIP_SNTP_MAX_SERVERS > 3
    esp_sntp_setservername(3, "0.pool.ntp.org");
#endif
    esp_sntp_set_time_sync_notification_cb(rtc_time_sync_notification_cb);
    esp_sntp_init();
}

// static void rtc_wait_for_sync(void)
// {
//     time_t now = 0;
//     struct tm timeinfo = {0};

//     int retry = 0;

//     while (timeinfo.tm_year < (2023 - 1900) &&
//            ++retry < RTC_SYNC_RETRY_COUNT) {

//         ESP_LOGI(TAG, "Waiting for time sync... (%d/%d)",
//                  retry, RTC_SYNC_RETRY_COUNT);
        
//         vTaskDelay(pdMS_TO_TICKS(RTC_SYNC_RETRY_DELAY_MS));

//         time(&now);
//         localtime_r(&now, &timeinfo);
//     }

//     if (timeinfo.tm_year >= (2023 - 1900)) {
//         ESP_LOGI(TAG, "Time synchronized successfully");
        

//     } else {
//         ESP_LOGW(TAG, "Time sync failed");
//         i2c_lcd_clear();
//         i2c_lcd_set_cursor(0,0);
//         i2c_lcd_send_string("Time failed");
//     }
// }

static bool rtc_wait_for_sync(void)
{
    int elapsed_ms = 0;
    int next_log_ms = 0;

    while (elapsed_ms < RTC_SYNC_TIMEOUT_MS) {
        if (rtc_time_synced ||
            esp_sntp_get_sync_status() == SNTP_SYNC_STATUS_COMPLETED) {
            ESP_LOGI(TAG, "Time synchronized successfully");
            lcd_display_text(0, 0, "Time Synced", true);
            return true;
        }

        if (elapsed_ms >= next_log_ms) {
            ESP_LOGI(TAG, "Waiting for time sync... (%d/%d)",
                     elapsed_ms / RTC_SYNC_LOG_MS + 1,
                     RTC_SYNC_LOG_COUNT);
            next_log_ms += RTC_SYNC_LOG_MS;
        }

        vTaskDelay(pdMS_TO_TICKS(RTC_SYNC_POLL_MS));
        elapsed_ms += RTC_SYNC_POLL_MS;
    }

    if (rtc_is_time_valid()) {
        ESP_LOGW(TAG, "SNTP timeout, using already valid system time");
        lcd_display_text(0, 0, "Time Valid", true);
        return true;
    }

    ESP_LOGW(TAG, "Time sync failed");
    lcd_display_text(0, 0, "Time Failed", true);
    return false;
}

// Public APIs                   //
bool wifi_rtc_init(void)
{
    ESP_LOGI(TAG, "RTC Init Started");

    // Set Indian Timezone (IST) //
    setenv("TZ", "IST-5:30", 1);
    tzset();

    rtc_initialize_sntp();
    bool time_ready = rtc_wait_for_sync();

    char timestr[32]; 
    rtc_get_time_string(timestr, sizeof(timestr));
    if (time_ready) {
        printf("Current time: %s\n", timestr);
        lcd_display_text(0, 0, "Time:", true);
        lcd_display_text(1, 0, timestr, false);
    } else {
        printf("Current time unavailable\n");
    }

    // if (xSemaphoreTake(lcd_mutex, pdMS_TO_TICKS(20))){
    //     i2c_lcd_clear();
    //     i2c_lcd_set_cursor(0,0);
    //     i2c_lcd_send_string("Time:");
    //     i2c_lcd_set_cursor(1,0);
    //     i2c_lcd_send_string(timestr);
    //     xSemaphoreGive(lcd_mutex);
    // }
    return time_ready;
}

bool rtc_is_time_valid(void)
{
    time_t now;
    struct tm timeinfo;

    time(&now);
    localtime_r(&now, &timeinfo);

    return (timeinfo.tm_year >= (RTC_MIN_VALID_YEAR - 1900));
}

void rtc_get_time(struct tm *timeinfo)
{
    if (timeinfo == NULL) return;

    time_t now;
    time(&now);
    localtime_r(&now, timeinfo);
}

void rtc_get_time_string(char *buffer, size_t max_len)
{
    if (buffer == NULL) return;

    if (!rtc_is_time_valid()) {
        snprintf(buffer, max_len, "Time not set");
        return;
    }

    struct tm timeinfo;
    rtc_get_time(&timeinfo);

    strftime(buffer, max_len, "%d-%m-%Y %H:%M", &timeinfo);
}

void rtc_resync(void)
{
    ESP_LOGI(TAG, "Resynchronizing time");
    if (esp_sntp_enabled()) {
        esp_sntp_stop();
    }
    rtc_initialize_sntp();
    rtc_wait_for_sync();
}




// #include "rtc.h"
// #include "esp_sntp.h"
// #include "esp_log.h"
// #include <string.h>
// #include <sys/time.h>

// static const char *TAG = "RTC";

// static bool time_synced = false;

// /* ---------------- SNTP CALLBACK ---------------- */
// static void time_sync_notification_cb(struct timeval *tv)
// {
//     ESP_LOGI(TAG, "Time synchronized");
//     time_synced = true;
// }

// /* ---------------- INIT ---------------- */
// void wifi_rtc_init(void)
// {
//     ESP_LOGI(TAG, "Initializing RTC (SNTP)");

//     setenv("TZ", "IST-5:30", 1);
//     tzset();

//     esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
//     esp_sntp_setservername(0, "pool.ntp.org");

//     esp_sntp_set_time_sync_notification_cb(time_sync_notification_cb);

//     esp_sntp_init();
// }

// /* ---------------- STATUS ---------------- */
// bool rtc_is_synced(void)
// {
//     if (time_synced) return true;

//     // fallback check
//     time_t now;
//     time(&now);

//     struct tm timeinfo;
//     localtime_r(&now, &timeinfo);

//     if (timeinfo.tm_year >= (2023 - 1900)) {
//         time_synced = true;
//         return true;
//     }

//     return false;
// }

// /* ---------------- GET TIME ---------------- */
// void rtc_get_time(struct tm *timeinfo)
// {
//     time_t now;
//     time(&now);
//     localtime_r(&now, timeinfo);
// }

// /* ---------------- GET STRING ---------------- */
// void rtc_get_time_string(char *buffer, int max_len)
// {
//     struct tm timeinfo;
//     rtc_get_time(&timeinfo);

//     strftime(buffer, max_len, "%d-%m-%Y %H:%M:%S", &timeinfo);
// }
