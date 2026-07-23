#include "rtc.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"

static const char *TAG = "RTC";

// Global time variable
rtc_time_t current_time = {25, 9, 8, 12, 0, 0};

// Days in each month
static const int days_in_month[] = {31,28,31,30,31,30,31,31,30,31,30,31};

bool is_leap_year(int year) {
    return ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0));
}

void increment_time() {
    current_time.sec++;
    if (current_time.sec >= 60) {
        current_time.sec = 0;
        current_time.min++;
    }
    if (current_time.min >= 60) {
        current_time.min = 0;
        current_time.hours++;
    }
    if (current_time.hours >= 24) {
        current_time.hours = 0;
        current_time.day++;
    }

    int dim = days_in_month[current_time.month - 1];
    if (current_time.month == 2 && is_leap_year(current_time.year)) {
        dim = 29;
    }

    if (current_time.day > dim) {
        current_time.day = 1;
        current_time.month++;
    }
    if (current_time.month > 12) {
        current_time.month = 1;
        current_time.year++;
    }
}

void save_time_on_nvs(void) {
    nvs_handle_t handle;
    if (nvs_open("storage", NVS_READWRITE, &handle) == ESP_OK) {
        nvs_set_blob(handle, "rtc_time", &current_time, sizeof(current_time));
        nvs_commit(handle);
        nvs_close(handle);
        //ESP_LOGI(TAG, "RTC time saved to NVS");
    }
}

void load_time_from_nvs(void) {
    nvs_handle_t handle;
    size_t required_size = sizeof(current_time);
    if (nvs_open("storage", NVS_READWRITE, &handle) == ESP_OK) {
        if (nvs_get_blob(handle, "rtc_time", &current_time, &required_size) == ESP_OK) {
            ESP_LOGI(TAG, "RTC time restored from NVS");
        }
        nvs_close(handle);
    }
}

void rtc_task(void *pvParameters) {
    while (1) {
        increment_time();
        save_time_on_nvs();
        vTaskDelay(pdMS_TO_TICKS(1000)); // 1 second tick
    }
}

void app_rtc_init(void) {
    load_time_from_nvs();
    xTaskCreate(rtc_task, "rtc_task", 2048, NULL, 5, NULL);
}

void rtc_get_time_str(char *buffer, int len) {
    snprintf(buffer, len, "%02d-%02d-%02d %02d:%02d",
            //  current_time.year, current_time.month, current_time.day,
            //  current_time.hours, current_time.min, current_time.sec);
            current_time.day,current_time.month, current_time.year,
            current_time.hours, current_time.min);
}
