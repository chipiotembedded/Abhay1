#pragma once

#include <stdbool.h>
#include "lora_receive.h"
#include "esp_err.h"

#define MAX_OWNERS 50
#define LFS_FILE_PATH "/littlefs/test.csv"

// typedef struct {
//     char name[64];
//     char flat[32];
//     char mac[32];
// } FlatOwner;

#ifdef __cplusplus
extern "C" {
#endif

extern int total_owners;
extern int current_owner;
extern FlatOwner owners[MAX_OWNERS];

esp_err_t flash_csv_init(void);
void load_owners_from_csv(void);
void show_owner_slide(int index);
// void flat_owner_nav_task(void *arg);

#ifdef __cplusplus
}
#endif



