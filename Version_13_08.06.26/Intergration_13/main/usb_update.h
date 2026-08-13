#ifndef USB_UPDATE_H
#define USB_UPDATE_H

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <ctype.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "esp_err.h"
#include "usb/usb_host.h"
#include "usb/msc_host.h"
#include "usb/msc_host_vfs.h"
#include "esp_littlefs.h"
#include "lcd_i2c.h"
#include "password.h"
#include "lora_receive.h"
#include "log_manager.h"
#include "wifi_manager.h"

// Configuration
#define USB_FILE_PATH "/usb/test.csv"
#define LFS_FILE_PATH "/littlefs/test.csv"

// Global variables

extern int total_owners;
extern int current_owner;

// Function declarations
void usb_update_task(void *arg);
void load_owners_from_csv(void);
//void load_pass_from_txt(const char *path1);
void show_owner_slide(int index);
esp_err_t mount_littlefs(void);

#endif // USB_UPDATE_H