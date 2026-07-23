// flash_csv.h
#ifndef FLASH_CSV_H
#define FLASH_CSV_H

#include "esp_err.h"

esp_err_t flash_csv_init(void);
esp_err_t flash_password_init(void);
esp_err_t flash_wifi_init(void);
esp_err_t flash_all_defaults(void);

#endif