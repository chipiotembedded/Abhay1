#include "git_hub_uploader.h"

#define GITHUB_USERNAME  "chipiotembedded"
#define GITHUB_REPO      "Abhay"
##define GITHUB_TOKEN "<SET_YOUR_TOKEN_HERE>"

static const char *TAG = "GITHUB_UPLOADER";

/* ------------------------------------------------------------------ */
/*  Init                                                               */
/* ------------------------------------------------------------------ */

esp_err_t github_uploader_init(void)
{
    ESP_LOGI(TAG, "GitHub uploader initialized");
    return ESP_OK;
}

/* ------------------------------------------------------------------ */
/*  Private helper: read file → base64-encode → HTTP PUT to GitHub    */
/*                                                                     */
/*  filepath        : LittleFS path of the file to upload             */
/*  filename_fmt    : strftime format string for the remote filename   */
/*                    e.g. "(Log)%d-%m-%Y_%H:%M.txt"                  */
/* ------------------------------------------------------------------ */

static esp_err_t github_upload_file(const char *filepath,
                                    const char *filename_fmt)
{
    /* ---------- build timestamped remote filename ---------- */
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);

    ESP_LOGI(TAG, "Time: %04d-%02d-%02d %02d:%02d",
             timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
             timeinfo.tm_hour, timeinfo.tm_min);

    char filename[64];
    strftime(filename, sizeof(filename), filename_fmt, &timeinfo);
    ESP_LOGI(TAG, "Uploading '%s' as '%s'", filepath, filename);

    /* ---------- read file into buffer ---------- */
    FILE *f = fopen(filepath, "r");
    if (!f) {
        ESP_LOGE(TAG, "Failed to open %s", filepath);
        return ESP_FAIL;
    }

    fseek(f, 0, SEEK_END);
    long file_size = ftell(f);
    rewind(f);

    if (file_size <= 0) {
        ESP_LOGW(TAG, "File '%s' is empty, skipping upload", filepath);
        fclose(f);
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "File size: %ld bytes", file_size);

    char *buffer = malloc(file_size);
    if (!buffer) {
        ESP_LOGE(TAG, "malloc failed for file buffer (%ld bytes)", file_size);
        fclose(f);
        return ESP_ERR_NO_MEM;
    }

    size_t bytes_read = fread(buffer, 1, file_size, f);
    fclose(f);

    if (bytes_read != (size_t)file_size) {
        ESP_LOGE(TAG, "Short read on '%s' (%u / %ld bytes)",
                 filepath, (unsigned)bytes_read, file_size);
        free(buffer);
        return ESP_FAIL;
    }

    /* ---------- base64-encode with mbedTLS ---------- */
    size_t encoded_len = 0;
    mbedtls_base64_encode(NULL, 0, &encoded_len,
                          (unsigned char *)buffer, file_size);

    char *b64 = malloc(encoded_len + 1);
    if (!b64) {
        ESP_LOGE(TAG, "malloc failed for base64 buffer");
        free(buffer);
        return ESP_ERR_NO_MEM;
    }

    int ret = mbedtls_base64_encode((unsigned char *)b64, encoded_len,
                                    &encoded_len,
                                    (unsigned char *)buffer, file_size);
    free(buffer);

    if (ret != 0) {
        ESP_LOGE(TAG, "Base64 encoding failed (ret=%d)", ret);
        free(b64);
        return ESP_FAIL;
    }
    b64[encoded_len] = '\0';
    ESP_LOGI(TAG, "Base64 size: %lu bytes", (unsigned long)encoded_len);

    /* ---------- build JSON body ---------- */
    size_t json_size = encoded_len + 200;
    char *json_body = malloc(json_size);
    if (!json_body) {
        ESP_LOGE(TAG, "malloc failed for JSON body");
        free(b64);
        return ESP_ERR_NO_MEM;
    }

    snprintf(json_body, json_size,
             "{\"message\":\"ESP32 log upload\",\"content\":\"%s\"}",
             b64);
    free(b64);

    ESP_LOGI(TAG, "JSON size: %lu bytes", (unsigned long)strlen(json_body));

    /* ---------- HTTP PUT to GitHub Contents API ---------- */
    char url[256];
    snprintf(url, sizeof(url),
             "https://api.github.com/repos/%s/%s/contents/%s",
             GITHUB_USERNAME, GITHUB_REPO, filename);
    ESP_LOGI(TAG, "PUT %s", url);

    esp_http_client_config_t config = {
        .url             = url,
        .method          = HTTP_METHOD_PUT,
        .timeout_ms      = 20000,
        .crt_bundle_attach = esp_crt_bundle_attach,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_http_client_set_header(client, "Authorization", "token " GITHUB_TOKEN);
    esp_http_client_set_header(client, "User-Agent",    "ESP32");
    esp_http_client_set_header(client, "Content-Type",  "application/json");
    esp_http_client_set_post_field(client, json_body, strlen(json_body));

    esp_err_t err = esp_http_client_perform(client);

    if (err == ESP_OK) {
        int status = esp_http_client_get_status_code(client);
        ESP_LOGI(TAG, "GitHub response status: %d", status);
    } else {
        ESP_LOGE(TAG, "HTTP PUT failed: %s", esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);
    free(json_body);

    return (err == ESP_OK) ? ESP_OK : ESP_FAIL;
}

/* ------------------------------------------------------------------ */
/*  Public API                                                         */
/* ------------------------------------------------------------------ */

esp_err_t github_upload_log(void)
{
    return github_upload_file(LOG_FILE_PATH,
                              "(Log)%d-%m-%Y_%H:%M.txt");
}

esp_err_t github_upload_activity_log(void)
{
    return github_upload_file(ACTIVITY_FILE_PATH,
                              "(Activity_Log)%d-%m-%Y_%H:%M.txt");
}
