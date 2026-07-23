#ifndef LORA_RECEIVE_H
#define LORA_RECEIVE_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_vfs.h"
#include "esp_littlefs.h"
#include "esp_timer.h"   // for esp_timer_get_time()
#include "esp_task_wdt.h"

#include "lora.h"
#include "lcd_i2c.h"
#include "usb_update.h"
#include "activity_log.h"
#include "button_handler.h"
#include "rtc.h"
#include "screen_display.h"


// ================== Hardware Pins ==================
#define BUZZER_PIN 9
#define MAX_OWNERS 100
// ================== Flat Owner Info ==================
typedef struct {
    char name[32];
    char flat[16];
    char mac[20];
} FlatOwner;

// ================== Globals ==================
extern bool shouldBeep;
// extern QueueHandle_t lora_rx_queue;

// ================== Functions ==================
void receiver_start(void);                   // Start LoRa + tasks
FlatOwner* find_owner_by_mac(const char *);  // Lookup CSV
extern FlatOwner owners[MAX_OWNERS];
#endif // LORA_RECEIVE_H
