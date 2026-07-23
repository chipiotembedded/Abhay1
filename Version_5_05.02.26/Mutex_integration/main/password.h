#ifndef PASSWORD_H
#define PASSWORD_H

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "esp_err.h"

#define PASSWORD_LENGTH 4
#define USB_PASS_FILE "/usb/password.txt"
#define SPIFFS_PASS_FILE "/littlefs/password.txt"

extern int correct_password[PASSWORD_LENGTH];
extern int entered_password[PASSWORD_LENGTH];
extern int current_digit_index;
extern bool password_verified;

esp_err_t load_pass_from_txt(const char *path);

#endif
