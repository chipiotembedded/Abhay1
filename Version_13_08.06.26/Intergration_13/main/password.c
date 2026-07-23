#include "password.h"

static const char *TAG = "PASSWORD";

int correct_password[PASSWORD_LENGTH];

esp_err_t load_pass_from_usb_to_flash(void){
    //For Password
    FILE *src1 = fopen(PASS_USB_FILE, "r");
    if (!src1) {
        ESP_LOGE(TAG, "Failed to open USB file");
        return false;
    }

    FILE *dst1 = fopen(PASS_FILE_PATH, "w");
    if (!dst1) {
        ESP_LOGE(TAG, "Failed to create LFS file");
        fclose(src1);
        return false;
    }

    char buf1[128];
    size_t r1;
    while ((r1 = fread(buf1, 1, sizeof(buf1), src1)) > 0) {
        fwrite(buf1, 1, r1, dst1);
    }

    fclose(src1);
    fclose(dst1);
    ESP_LOGI(TAG, "Password file copied to LittleFS");

    return ESP_OK;
}

esp_err_t load_pass_from_txt(void)
{
    FILE *f = fopen(PASS_FILE_PATH, "r");
    if (!f) {
        ESP_LOGE(TAG, "Failed to open %s", PASS_FILE_PATH);
        return ESP_FAIL;
    }

    char line[PASSWORD_LENGTH + 2] = {0};

    if (!fgets(line, sizeof(line), f)) {
        fclose(f);
        return ESP_FAIL;
    }
    fclose(f);

    line[strcspn(line, "\r\n")] = 0;

    if (strlen(line) != PASSWORD_LENGTH) {
        return ESP_FAIL;
    }

    for (int i = 0; i < PASSWORD_LENGTH; i++) {
        if (line[i] < '0' || line[i] > '9') return ESP_FAIL;
        correct_password[i] = line[i] - '0';
    }

    ESP_LOGI(TAG, "Password loaded");
    return ESP_OK;
}