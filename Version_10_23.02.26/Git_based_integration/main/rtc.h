// #ifndef RTC_H
// #define RTC_H

// #include <stdbool.h>
// #include "freertos/FreeRTOS.h"

// // RTC structure
// typedef struct {
//     int year;
//     int month;
//     int day;
//     int hours;
//     int min;
//     int sec;
// } rtc_time_t;

// extern rtc_time_t current_time;

// // API functions
// void app_rtc_init(void);
// void rtc_task(void *pvParameters);
// void increment_time(void);
// bool is_leap_year(int year);
// void save_time_on_nvs(void);
// void load_time_from_nvs(void);
// void rtc_get_time_str(char *buffer, int len);

// #endif // RTC_H


#ifndef RTC_H
#define RTC_H

#include <stdbool.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

void wifi_rtc_init(void);
bool rtc_is_time_valid(void);
void rtc_get_time(struct tm *timeinfo);
void rtc_get_time_string(char *buffer, size_t max_len);
void rtc_resync(void);

#ifdef __cplusplus
}
#endif

#endif
