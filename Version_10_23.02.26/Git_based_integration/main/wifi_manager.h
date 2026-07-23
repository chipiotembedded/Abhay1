#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "esp_err.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_netif.h"
#include "rtc.h"

// #define USB_WIFI_CRED_PATH "/usb/wifi_credentials.txt"
// #define LFS_WIFI_CRED_PATH "/littlefs/wifi_credentials.txt"
// #define MAX_SSID_LEN   32
// #define MAX_PASS_LEN   64

esp_err_t wifi_manager_init(void);
bool wifi_manager_is_connected(void);
void wifi_manager_wait_for_connection(void);
// esp_err_t load_wifi_cred_from_usb_to_flash(void);
// esp_err_t load_wifi_cred_from_txt(void);
// void wifi_monitor_task(void *pv);

#endif