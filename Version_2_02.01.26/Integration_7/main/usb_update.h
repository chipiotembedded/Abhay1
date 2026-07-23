/*
#ifndef USB_UPDATE_H
#define USB_UPDATE_H

#include <stdbool.h>
#include "lora_receive.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "esp_err.h"

// Configuration
#define MAX_OWNERS 100
#define LFS_PATH "/littlefs"
#define USB_PATH "/usb"
#define USB_CSV_PATH "/usb/ESP/test.csv"
#define LFS_CSV_PATH "/littlefs/test.csv"

// App message structure for USB events
typedef struct {
    enum {
        APP_QUIT,
        APP_DEVICE_CONNECTED,
        APP_DEVICE_DISCONNECTED,
    } id;
    union {
        uint8_t new_dev_address;
    } data;
} app_message_t;

// Global variables
extern FlatOwner owners[MAX_OWNERS];
extern int total_owners;
extern int current_owner;
extern QueueHandle_t app_queue;

// Function declarations
void usb_update_task(void *arg);
bool usb_update_start(void);
bool usb_update_mount_littlefs(void);
void load_owners_from_csv(void);
void show_owner_slide(int index);

#endif // USB_UPDATE_H
*/



#ifndef USB_UPDATE_H
#define USB_UPDATE_H

#include <stdbool.h>
#include "lora_receive.h"

// Configuration
#define MAX_OWNERS 100
#define USB_FILE_PATH "/usb/ESP/test.csv"
#define LFS_FILE_PATH "/littlefs/test.csv"

// Global variables
extern FlatOwner owners[MAX_OWNERS];
extern int total_owners;
extern int current_owner;

// Function declarations
void usb_update_task(void *arg);
void load_owners_from_csv(const char *path);
void show_owner_slide(int index);

#endif // USB_UPDATE_H