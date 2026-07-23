/*
#include "git_hub_uploader.h"
#include "esp_log.h"
#include "esp_system.h"
#include <stdio.h>
#include <string.h>
#include "mbedtls/base64.h"
#include "esp_http_client.h"
#include "esp_crt_bundle.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include <stdlib.h>
#include "esp_sntp.h"
#include <time.h>
#include "esp_timer.h"
#include <sys/time.h>
#include "log_manager.h"

static EventGroupHandle_t wifi_event_group;
#define WIFI_CONNECTED_BIT BIT0
#define GITHUB_USERNAME   "chipiotembedded"
#define GITHUB_REPO       "Abhay"
#define GITHUB_TOKEN "<SET_YOUR_TOKEN_HERE>"

static void wifi_event_handler(void* arg,
                                esp_event_base_t event_base,
                                int32_t event_id,
                                void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
    ESP_LOGI("WIFI", "STA Start");
    }

    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {

        wifi_event_sta_disconnected_t *event =
            (wifi_event_sta_disconnected_t *) event_data;

        ESP_LOGE("WIFI", "Disconnected, reason=%d", event->reason);
        

        esp_wifi_connect();
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {

        ip_event_got_ip_t *event = (ip_event_got_ip_t*) event_data;

        ESP_LOGI("WIFI", "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));

        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
    }
}


void wifi_scan_task(void *pvParameters)
{
    ESP_LOGI("SCAN", "Starting WiFi scan...");

    wifi_scan_config_t scan_config = {
        .ssid = 0,
        .bssid = 0,
        .channel = 0,
        .show_hidden = true
    };

    esp_wifi_scan_start(&scan_config, true);

    uint16_t number = 20;
    wifi_ap_record_t *ap_info = malloc(sizeof(wifi_ap_record_t) * number);

    if (ap_info == NULL) {
        ESP_LOGE("SCAN", "Memory allocation failed");
        vTaskDelete(NULL);
        return;
    }

    esp_wifi_scan_get_ap_records(&number, ap_info);

    for (int i = 0; i < number; i++) {
        ESP_LOGI("SCAN", "SSID: %s | RSSI: %d | Channel: %d",
                 ap_info[i].ssid,
                 ap_info[i].rssi,
                 ap_info[i].primary);
    }

    free(ap_info);
    vTaskDelete(NULL);
}


void wifi_init_sta(void)
{
    wifi_event_group = xEventGroupCreate();

    nvs_flash_init();
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    esp_event_handler_instance_register(WIFI_EVENT,
                                        ESP_EVENT_ANY_ID,
                                        &wifi_event_handler,
                                        NULL,
                                        NULL);

    esp_event_handler_instance_register(IP_EVENT,
                                        IP_EVENT_STA_GOT_IP,
                                        &wifi_event_handler,
                                        NULL,
                                        NULL);

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = "Atharv's A35",
            .password = "12345678",
        },
    };

    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_wifi_start();

    // First scan
    xTaskCreate(wifi_scan_task, "wifi_scan", 4096, NULL, 5, NULL);

    // Wait for scan to finish
    vTaskDelay(pdMS_TO_TICKS(4000));

    ESP_LOGI("WIFI", "Connecting to WiFi...");
    esp_wifi_connect();


    xEventGroupWaitBits(wifi_event_group,
                        WIFI_CONNECTED_BIT,
                        pdFALSE,
                        pdTRUE,
                        portMAX_DELAY);
}



static const char *TAG = "GITHUB_UPLOADER";


// @brief Initialize uploader module

esp_err_t github_uploader_init(void)
{
    ESP_LOGI(TAG, "GitHub uploader initialized");
    return ESP_OK;
}


//  * @brief Upload log file (Skeleton version)

static char github_date[64];

static esp_err_t http_event_handler(esp_http_client_event_t *evt)
{
    if (evt->event_id == HTTP_EVENT_ON_HEADER) {

        if (strcasecmp(evt->header_key, "Date") == 0) {

            snprintf(github_date, sizeof(github_date),
                     "%s", evt->header_value);

            ESP_LOGI("TIME", "Captured Date header: %s", github_date);
        }
    }
    return ESP_OK;
}

static void sync_time_from_github(void)
{
    ESP_LOGI("TIME", "Syncing time from GitHub...");

    memset(github_date, 0, sizeof(github_date));

    esp_http_client_config_t config = {
        .url = "https://api.github.com",
        .method = HTTP_METHOD_GET,
        .event_handler = http_event_handler,
        .timeout_ms = 10000,
        .crt_bundle_attach = esp_crt_bundle_attach,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);

    esp_err_t err = esp_http_client_perform(client);

    if (err == ESP_OK && strlen(github_date) > 0) {

        struct tm tm = {0};

        strptime(github_date, "%a, %d %b %Y %H:%M:%S", &tm);

        // STEP 1: Force UTC for correct conversion
        setenv("TZ", "UTC0", 1);
        tzset();

        time_t t = mktime(&tm);

        struct timeval now = {
            .tv_sec = t,
            .tv_usec = 0
        };

        settimeofday(&now, NULL);

        // STEP 2: Now set IST timezone
        setenv("TZ", "IST-5:30", 1);
        tzset();

        ESP_LOGI("TIME", "System time updated successfully");
    }
    else {
        ESP_LOGE("TIME", "Failed to sync time");
    }

    esp_http_client_cleanup(client);
}



esp_err_t github_upload_log(void)
{
    ESP_LOGI(TAG, "Starting Base64 test...");

    // Sync time first
    sync_time_from_github();

    // Now generate correct timestamp
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    ESP_LOGI("TIME", "Year now = %d", timeinfo.tm_year + 1900);
    ESP_LOGI("TIME", "Hour now = %d", timeinfo.tm_hour);
    char filename[64];
    strftime(filename, sizeof(filename), "%Y-%m-%d_%H-%M-%S.txt", &timeinfo);

    ESP_LOGI(TAG, "Uploading as: %s", filename);

    FILE *f = fopen(LOG_FILE_PATH, "r");
    if (!f) {
        ESP_LOGE(TAG, "Failed to open log file");
        return ESP_FAIL;
    }

    fseek(f, 0, SEEK_END);
    long file_size = ftell(f);
    rewind(f);

    if (file_size <= 0) {
        ESP_LOGW(TAG, "Log file empty");
        fclose(f);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "File size: %ld bytes", file_size);

    char *buffer = malloc(file_size);
    if (!buffer) {
        ESP_LOGE(TAG, "File buffer malloc failed for %ld bytes", file_size);
        fclose(f);
        return ESP_ERR_NO_MEM;
    }

    size_t bytes_read = fread(buffer, 1, file_size, f);
    fclose(f);
    if (bytes_read != (size_t)file_size) {
        ESP_LOGE(TAG, "Failed to read complete log file (%u/%ld bytes)",
                 (unsigned int)bytes_read, file_size);
        free(buffer);
        return ESP_FAIL;
    }

    // --- BASE64 ENCODING USING MBEDTLS ---
                       
size_t encoded_len = 0;

// First call to get required length
mbedtls_base64_encode(NULL, 0, &encoded_len,
                      (unsigned char*)buffer, file_size);

char *base64_output = malloc(encoded_len + 1);
if (!base64_output) {
    ESP_LOGE(TAG, "Base64 malloc failed");
    free(buffer);
    return ESP_ERR_NO_MEM;
}

// Actual encoding
int ret = mbedtls_base64_encode((unsigned char*)base64_output,
                                encoded_len,
                                &encoded_len,
                                (unsigned char*)buffer,
                                file_size);

if (ret != 0) {
    ESP_LOGE(TAG, "Base64 encoding failed");
    free(buffer);
    free(base64_output);
    return ESP_FAIL;
}

base64_output[encoded_len] = '\0';

ESP_LOGI(TAG, "Base64 size: %lu bytes", (unsigned long)encoded_len);
ESP_LOGI(TAG, "Base64 preview: %.100s", base64_output);

// ---- CREATE JSON BODY ----

const char *commit_message = "ESP32 log upload";

size_t json_size = strlen(base64_output) + 200;

char *json_body = malloc(json_size);
if (!json_body) {
    ESP_LOGE(TAG, "JSON malloc failed");
    free(buffer);
    free(base64_output);
    return ESP_ERR_NO_MEM;
}

snprintf(json_body, json_size,
         "{"
         "\"message\":\"%s\","
         "\"content\":\"%s\""
         "}",
         commit_message,
         base64_output);

ESP_LOGI(TAG, "JSON size: %lu", (unsigned long)strlen(json_body));
ESP_LOGI(TAG, "JSON preview: %.150s", json_body);

//===================== github upload ========================//
ESP_LOGI(TAG, "Uploading to GitHub...");

// Build GitHub URL
char url[256];
snprintf(url, sizeof(url),
         "https://api.github.com/repos/%s/%s/contents/%s",
         GITHUB_USERNAME,
         GITHUB_REPO,
         filename);

ESP_LOGI(TAG, "URL: %s", url);

esp_http_client_config_t config = {
    .url = url,
    .method = HTTP_METHOD_PUT,
    .timeout_ms = 20000,
    .crt_bundle_attach = esp_crt_bundle_attach,
};

esp_http_client_handle_t client = esp_http_client_init(&config);

// Required headers for GitHub
esp_http_client_set_header(client, "Authorization", "Bearer " GITHUB_TOKEN);
esp_http_client_set_header(client, "User-Agent", "ESP32");
esp_http_client_set_header(client, "Content-Type", "application/json");

esp_http_client_set_post_field(client, json_body, strlen(json_body));

esp_err_t err = esp_http_client_perform(client);


//============== upload check =================//
if (err == ESP_OK) {
    int status = esp_http_client_get_status_code(client);
    ESP_LOGI(TAG, "GitHub Status = %d", status);
} else {
    ESP_LOGE(TAG, "GitHub upload failed: %s", esp_err_to_name(err));
}

esp_http_client_cleanup(client);


free(buffer);
free(base64_output);
free(json_body);

return ESP_OK;

}
*/



#include "git_hub_uploader.h"

#define GITHUB_USERNAME   "chipiotembedded"
#define GITHUB_REPO       "Abhay"
#define GITHUB_TOKEN "<SET_YOUR_TOKEN_HERE>"

static const char *TAG = "GITHUB_UPLOADER";


//@brief Initialize uploader module//

esp_err_t github_uploader_init(void)
{
    ESP_LOGI(TAG, "GitHub uploader initialized");
    return ESP_OK;
}

esp_err_t github_upload_log(void)
{
    ESP_LOGI(TAG, "Starting Base64 test...");    

    // Now generate correct timestamp
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    ESP_LOGI("TIME", "Year now = %d", timeinfo.tm_year + 1900);
    ESP_LOGI("TIME", "Hour now = %d", timeinfo.tm_hour);
    char filename[64];
    strftime(filename, sizeof(filename), "(Log)%d-%m-%Y_%H:%M.txt", &timeinfo);

    ESP_LOGI(TAG, "Uploading as: %s", filename);

    FILE *f = fopen(LOG_FILE_PATH, "r");
    if (!f) {
        ESP_LOGE(TAG, "Failed to open log file");
        return ESP_FAIL;
    }

    fseek(f, 0, SEEK_END);
    long file_size = ftell(f);
    rewind(f);

    if (file_size <= 0) {
        ESP_LOGW(TAG, "Log file empty");
        fclose(f);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "File size: %ld bytes", file_size);

    char *buffer = malloc(file_size);
    if (!buffer) {
        ESP_LOGE(TAG, "Log buffer malloc failed for %ld bytes", file_size);
        fclose(f);
        return ESP_ERR_NO_MEM;
    }

    size_t bytes_read = fread(buffer, 1, file_size, f);
    fclose(f);
    if (bytes_read != (size_t)file_size) {
        ESP_LOGE(TAG, "Failed to read complete log file (%u/%ld bytes)",
                 (unsigned int)bytes_read, file_size);
        free(buffer);
        return ESP_FAIL;
    }

    // --- BASE64 ENCODING USING MBEDTLS ---
                       
size_t encoded_len = 0;

// First call to get required length
mbedtls_base64_encode(NULL, 0, &encoded_len,
                      (unsigned char*)buffer, file_size);

char *base64_output = malloc(encoded_len + 1);
if (!base64_output) {
    ESP_LOGE(TAG, "Base64 malloc failed");
    free(buffer);
    return ESP_ERR_NO_MEM;
}

// Actual encoding
int ret = mbedtls_base64_encode((unsigned char*)base64_output,
                                encoded_len,
                                &encoded_len,
                                (unsigned char*)buffer,
                                file_size);

if (ret != 0) {
    ESP_LOGE(TAG, "Base64 encoding failed");
    free(buffer);
    free(base64_output);
    return ESP_FAIL;
}

base64_output[encoded_len] = '\0';

ESP_LOGI(TAG, "Base64 size: %lu bytes", (unsigned long)encoded_len);
ESP_LOGI(TAG, "Base64 preview: %.100s", base64_output);

// ---- CREATE JSON BODY ----

const char *commit_message = "ESP32 log upload";

size_t json_size = strlen(base64_output) + 200;

char *json_body = malloc(json_size);
if (!json_body) {
    ESP_LOGE(TAG, "JSON malloc failed");
    free(buffer);
    free(base64_output);
    return ESP_ERR_NO_MEM;
}

snprintf(json_body, json_size,
         "{"
         "\"message\":\"%s\","
         "\"content\":\"%s\""
         "}",
         commit_message,
         base64_output);

ESP_LOGI(TAG, "JSON size: %lu", (unsigned long)strlen(json_body));
ESP_LOGI(TAG, "JSON preview: %.150s", json_body);

/*===================== github upload ========================*/
ESP_LOGI(TAG, "Uploading to GitHub...");

// Build GitHub URL
char url[256];
snprintf(url, sizeof(url),
         "https://api.github.com/repos/%s/%s/contents/%s",
         GITHUB_USERNAME,
         GITHUB_REPO,
         filename);

ESP_LOGI(TAG, "URL: %s", url);

esp_http_client_config_t config = {
    .url = url,
    .method = HTTP_METHOD_PUT,
    .timeout_ms = 20000,
    .crt_bundle_attach = esp_crt_bundle_attach,
};

esp_http_client_handle_t client = esp_http_client_init(&config);

// Required headers for GitHub
esp_http_client_set_header(client, "Authorization", "Bearer " GITHUB_TOKEN);
esp_http_client_set_header(client, "User-Agent", "ESP32");
esp_http_client_set_header(client, "Content-Type", "application/json");

esp_http_client_set_post_field(client, json_body, strlen(json_body));

esp_err_t err = esp_http_client_perform(client);


/*============== upload check =================*/
if (err == ESP_OK) {
    int status = esp_http_client_get_status_code(client);
    ESP_LOGI(TAG, "GitHub Status = %d", status);
} else {
    ESP_LOGE(TAG, "GitHub upload failed: %s", esp_err_to_name(err));
}

esp_http_client_cleanup(client);


free(buffer);
free(base64_output);
free(json_body);

return ESP_OK;

}

///
///Activity Log upload to git///
///

esp_err_t github_upload_activity_log(void)
{
    ESP_LOGI(TAG, "Starting Base64 test...");   

    // Now generate correct timestamp
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    ESP_LOGI("TIME", "Year now = %d", timeinfo.tm_year + 1900);
    ESP_LOGI("TIME", "Hour now = %d", timeinfo.tm_hour);
    char filename[64];
    strftime(filename, sizeof(filename), "(Activity_Log)%d-%m-%Y_%H:%M.txt", &timeinfo);

    ESP_LOGI(TAG, "Uploading as: %s", filename);

    FILE *f = fopen(ACTIVITY_FILE_PATH, "r");
    if (!f) {
        ESP_LOGE(TAG, "Failed to open log file");
        return ESP_FAIL;
    }

    fseek(f, 0, SEEK_END);
    long file_size = ftell(f);
    rewind(f);

    if (file_size <= 0) {
        ESP_LOGW(TAG, "Activity Log file empty");
        fclose(f);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "File size: %ld bytes", file_size);

    char *buffer = malloc(file_size);
    if (!buffer) {
        ESP_LOGE(TAG, "Activity buffer malloc failed for %ld bytes", file_size);
        fclose(f);
        return ESP_ERR_NO_MEM;
    }

    size_t bytes_read = fread(buffer, 1, file_size, f);
    fclose(f);
    if (bytes_read != (size_t)file_size) {
        ESP_LOGE(TAG, "Failed to read complete activity file (%u/%ld bytes)",
                 (unsigned int)bytes_read, file_size);
        free(buffer);
        return ESP_FAIL;
    }

    // --- BASE64 ENCODING USING MBEDTLS ---
                       
size_t encoded_len = 0;

// First call to get required length
mbedtls_base64_encode(NULL, 0, &encoded_len,
                      (unsigned char*)buffer, file_size);

char *base64_output = malloc(encoded_len + 1);
if (!base64_output) {
    ESP_LOGE(TAG, "Base64 malloc failed");
    free(buffer);
    return ESP_ERR_NO_MEM;
}

// Actual encoding
int ret = mbedtls_base64_encode((unsigned char*)base64_output,
                                encoded_len,
                                &encoded_len,
                                (unsigned char*)buffer,
                                file_size);

if (ret != 0) {
    ESP_LOGE(TAG, "Base64 encoding failed");
    free(buffer);
    free(base64_output);
    return ESP_FAIL;
}

base64_output[encoded_len] = '\0';

ESP_LOGI(TAG, "Base64 size: %lu bytes", (unsigned long)encoded_len);
ESP_LOGI(TAG, "Base64 preview: %.100s", base64_output);

// ---- CREATE JSON BODY ----

const char *commit_message = "ESP32 log upload";

size_t json_size = strlen(base64_output) + 200;

char *json_body = malloc(json_size);
if (!json_body) {
    ESP_LOGE(TAG, "JSON malloc failed");
    free(buffer);
    free(base64_output);
    return ESP_ERR_NO_MEM;
}

snprintf(json_body, json_size,
         "{"
         "\"message\":\"%s\","
         "\"content\":\"%s\""
         "}",
         commit_message,
         base64_output);

ESP_LOGI(TAG, "JSON size: %lu", (unsigned long)strlen(json_body));
ESP_LOGI(TAG, "JSON preview: %.150s", json_body);

/*===================== github upload ========================*/
ESP_LOGI(TAG, "Uploading to GitHub...");

// Build GitHub URL
char url[256];
snprintf(url, sizeof(url),
         "https://api.github.com/repos/%s/%s/contents/%s",
         GITHUB_USERNAME,
         GITHUB_REPO,
         filename);

ESP_LOGI(TAG, "URL: %s", url);

esp_http_client_config_t config = {
    .url = url,
    .method = HTTP_METHOD_PUT,
    .timeout_ms = 20000,
    .crt_bundle_attach = esp_crt_bundle_attach,
};

esp_http_client_handle_t client = esp_http_client_init(&config);

// Required headers for GitHub
esp_http_client_set_header(client, "Authorization", "Bearer " GITHUB_TOKEN);
esp_http_client_set_header(client, "User-Agent", "ESP32");
esp_http_client_set_header(client, "Content-Type", "application/json");

esp_http_client_set_post_field(client, json_body, strlen(json_body));

esp_err_t err = esp_http_client_perform(client);


/*============== upload check =================*/
if (err == ESP_OK) {
    int status = esp_http_client_get_status_code(client);
    ESP_LOGI(TAG, "GitHub Status = %d", status);
} else {
    ESP_LOGE(TAG, "GitHub upload failed: %s", esp_err_to_name(err));
}

esp_http_client_cleanup(client);


free(buffer);
free(base64_output);
free(json_body);

return ESP_OK;

}
