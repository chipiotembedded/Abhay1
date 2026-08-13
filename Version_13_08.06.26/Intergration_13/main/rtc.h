#ifndef RTC_H
#define RTC_H

#include <stdbool.h>
#include <time.h>
#include "lcd_i2c.h"

#ifdef __cplusplus
extern "C" {
#endif

bool wifi_rtc_init(void);
bool rtc_is_time_valid(void);
void rtc_get_time(struct tm *timeinfo);
void rtc_get_time_string(char *buffer, size_t max_len);
void rtc_resync(void);

#ifdef __cplusplus
}
#endif

#endif
