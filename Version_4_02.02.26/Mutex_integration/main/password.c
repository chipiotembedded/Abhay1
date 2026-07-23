#include "password.h"

static const char *TAG = "PASSWORD";

int correct_password[PASSWORD_LENGTH];

esp_err_t load_pass_from_txt(const char *path)
{
    FILE *f = fopen(path, "r");
    if (!f) {
        ESP_LOGE(TAG, "Failed to open %s", path);
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
