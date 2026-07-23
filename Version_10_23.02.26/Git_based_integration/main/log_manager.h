#ifndef LOG_MANAGER_H
#define LOG_MANAGER_H

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "esp_log.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include <stdarg.h>
#include <time.h>
#include "lcd_i2c.h"

#define LOG_FILE_PATH "/littlefs/log.txt"
#define LOG_USB_PATH "/usb/log.txt"
#define BUF_SIZE     1024

void log_manager_init(void);
// void log_check_timeout_task(void *pvParameters);
// void print_log_file(void);
esp_err_t copy_log_to_usb(void);

#endif
