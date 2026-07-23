// activity_log.h
#ifndef ACTIVITY_LOG_H
#define ACTIVITY_LOG_H

#include <stdbool.h>
#include <time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "system_states.h"

#define ACTIVITY_LOG_TAG  "ACTIVITY_LOG"
#define NVS_NAMESPACE "lora_logs"
#define MAX_LOGS 100

// Log entry structure
typedef struct {
    bool buttonPressed;
    char OwnerName[16];
    char FlatNumber[16];
    // time_t timestamp;
    char timestamp[20];
} log_entry;

extern SemaphoreHandle_t nvs_mutex;
extern int total_logs;
extern int current_log_index;

// Function declarations
void initialize_activity_log();
void log_received_data(bool buttonPressed, const char *ownerName, const char *flatNumber);
int get_total_logs();
int read_log_entry(int index, log_entry *entry);
void display_log_entry(int index);
void debug_print_all_logs();

#endif // ACTIVITY_LOG_H