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
