#ifndef RTC_H
#define RTC_H

#include <stdbool.h>
#include "freertos/FreeRTOS.h"

// RTC structure
typedef struct {
    int year;
    int month;
    int day;
    int hours;
    int min;
    int sec;
} rtc_time_t;

extern rtc_time_t current_time;

// API functions
void app_rtc_init(void);
void rtc_task(void *pvParameters);
void increment_time(void);
bool is_leap_year(int year);
void save_time_on_nvs(void);
void load_time_from_nvs(void);
void rtc_get_time_str(char *buffer, int len);

#endif // RTC_H
