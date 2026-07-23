// activity_log.h
#ifndef ACTIVITY_LOG_H
#define ACTIVITY_LOG_H

#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include "esp_littlefs.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "button_handler.h"
#include "lcd_i2c.h"
#include "rtc.h"

#define ACTIVITY_FILE_PATH   "/littlefs/activity.txt"
#define ACTIVITY_USB_FILE_PATH "/usb/activity.txt"
#define COPY_BUFFER_SIZE     512
#define ACTIVITY_LOG_TAG  "ACTIVITY_LOG"
#define MAX_LOGS 100

// Log entry structure
typedef struct {
    bool buttonPressed;
    char OwnerName[16];
    char FlatNumber[16];
    char timestamp[20];
} log_entry;

extern int total_logs;
extern int current_log_index;

// Function declarations
void initialize_activity_log();
void log_received_data(bool buttonPressed, const char *ownerName, const char *flatNumber);
int get_total_logs();
int read_log_entry(int index, log_entry *entry);
void display_log_entry(int index);
void clear_activity_logs(void);
void debug_print_all_logs();
esp_err_t copy_activity_to_usb(void);

#endif // ACTIVITY_LOG_H