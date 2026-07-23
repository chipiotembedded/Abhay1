#ifndef GITHUB_UPLOADER_H
#define GITHUB_UPLOADER_H

#include "esp_log.h"
#include "esp_system.h"
#include <stdio.h>
#include <string.h>
#include "mbedtls/base64.h"
#include "esp_http_client.h"
#include "esp_crt_bundle.h"
#include "esp_log.h"
#include <stdlib.h>
#include "log_manager.h"
#include "esp_err.h"
#include "lcd_i2c.h"

esp_err_t github_uploader_init(void);
esp_err_t github_upload_log(void);

#endif // GITHUB_UPLOADER_H
