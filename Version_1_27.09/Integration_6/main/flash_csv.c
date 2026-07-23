#include "flash_csv.h"
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <ctype.h>
#include "esp_log.h"
#include "esp_littlefs.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "lcd_i2c.h"
#include "system_states.h"

#define TAG "FLASH_CSV"

FlatOwner owners[MAX_OWNERS];
int total_owners = 0;
int current_owner = 0;

extern StateID current_state;
extern const uint8_t test_csv_start[] asm("_binary_test_csv_start");
extern const uint8_t test_csv_end[]   asm("_binary_test_csv_end");

// ---------- String helpers ----------
static inline void rtrim(char *s) {
    size_t n = strlen(s);
    while (n && isspace((unsigned char)s[n - 1])) s[--n] = '\0';
}
static inline char *ltrim(char *s) {
    while (*s && isspace((unsigned char)*s)) s++;
    return s;
}
static inline void trim_inplace(char *s) {
    char *p = ltrim(s);
    if (p != s) memmove(s, p, strlen(p) + 1);
    rtrim(s);
}

// --- Save embedded CSV into LittleFS ---
void save_embedded_csv() {
    FILE *f = fopen(LFS_FILE_PATH, "w");
    if (!f) {
        ESP_LOGE(TAG, "Failed to create %s", LFS_FILE_PATH);
        return;
    }
    size_t size = test_csv_end - test_csv_start;
    fwrite(test_csv_start, 1, size, f);
    fclose(f);
    ESP_LOGI(TAG, "CSV written to LittleFS (%d bytes)", (int)size);
}

// ---------- CSV Loader ----------
void load_owners_from_csv(void) {
    total_owners = 0;

    FILE *f = fopen(LFS_FILE_PATH, "r");
    if (!f) {
        ESP_LOGE(TAG, "Failed to open %s", LFS_FILE_PATH);
        return;
    }

    ESP_LOGI(TAG, "Loading owners from %s", LFS_FILE_PATH);

    char line[256];
    long pos = ftell(f);

    // Skip header row if present
    if (fgets(line, sizeof(line), f)) {
        char tmp[256];
        strncpy(tmp, line, sizeof(tmp) - 1);
        tmp[sizeof(tmp) - 1] = 0;
        trim_inplace(tmp);
        if (!(strncasecmp(tmp, "name", 4) == 0 || strncasecmp(tmp, "mac", 3) == 0)) {
            fseek(f, pos, SEEK_SET);
        }
    } else {
        fclose(f);
        return;
    }

    // Read rows
    while (fgets(line, sizeof(line), f) && total_owners < MAX_OWNERS) {
        line[strcspn(line, "\r\n")] = 0; // strip newline
        char *t1 = strtok(line, ",");
        char *t2 = strtok(NULL, ",");
        char *t3 = strtok(NULL, ",");
        if (!(t1 && t2 && t3)) continue;

        trim_inplace(t1);
        trim_inplace(t2);
        trim_inplace(t3);

        // Guess column order
        const char *name = NULL, *flat = NULL, *mac = NULL;
        int colons_t1 = 0, colons_t2 = 0, colons_t3 = 0;
        for (const char *p = t1; *p; ++p) if (*p == ':') colons_t1++;
        for (const char *p = t2; *p; ++p) if (*p == ':') colons_t2++;
        for (const char *p = t3; *p; ++p) if (*p == ':') colons_t3++;

        if (colons_t1 >= 5) { mac = t1; name = t2; flat = t3; }
        else if (colons_t3 >= 5) { name = t1; flat = t2; mac = t3; }
        else { name = t1; flat = t2; mac = t3; }

        if (mac && name && flat) {
            strncpy(owners[total_owners].name, name, sizeof(owners[total_owners].name) - 1);
            strncpy(owners[total_owners].flat, flat, sizeof(owners[total_owners].flat) - 1);
            strncpy(owners[total_owners].mac, mac, sizeof(owners[total_owners].mac) - 1);

            owners[total_owners].name[sizeof(owners[total_owners].name) - 1] = 0;
            owners[total_owners].flat[sizeof(owners[total_owners].flat) - 1] = 0;
            owners[total_owners].mac[sizeof(owners[total_owners].mac) - 1] = 0;

            ESP_LOGI(TAG, "Row %d: name='%s' flat='%s' mac='%s'",
                     total_owners,
                     owners[total_owners].name,
                     owners[total_owners].flat,
                     owners[total_owners].mac);

            total_owners++;
        }
    }

    fclose(f);
    ESP_LOGI(TAG, "Loaded %d owners", total_owners);
}

// ---------- LCD ----------
void show_owner_slide(int index) {
    i2c_lcd_clear();

    if (total_owners == 0 || index < 0 || index >= total_owners) {
        i2c_lcd_set_cursor(0, 0);
        i2c_lcd_send_string("CSV empty!");
        return;
    }

    char line[17];

    // Line 1: Name (truncate if longer than 16)
    snprintf(line, sizeof(line), "%.16s", owners[index].name);
    i2c_lcd_set_cursor(0, 0);
    i2c_lcd_send_string(line);

    // Line 2: Flat
    snprintf(line, sizeof(line), "%.16s", owners[index].flat);
    i2c_lcd_set_cursor(1, 0);
    i2c_lcd_send_string(line);

    // Line 3–4: MAC (split if longer than 16)
    char mac_part1[17] = {0};
    char mac_part2[17] = {0};

    strncpy(mac_part1, owners[index].mac, 16);
    mac_part1[16] = '\0';

    if (strlen(owners[index].mac) > 16) {
        strncpy(mac_part2, owners[index].mac + 16, 16);
        mac_part2[16] = '\0';
    }

    i2c_lcd_set_cursor(2, 0);
    i2c_lcd_send_string(mac_part1);

    i2c_lcd_set_cursor(3, 0);
    i2c_lcd_send_string(mac_part2);
}

esp_err_t flash_csv_init(void) {
    // Mount LittleFS
    esp_vfs_littlefs_conf_t conf = {
        .base_path = "/littlefs",
        .partition_label = "storage",
        .format_if_mount_failed = true,
        .dont_mount = false
    };

    esp_err_t err = esp_vfs_littlefs_register(&conf);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to mount LittleFS: %s", esp_err_to_name(err));
        return err;
    }
    ESP_LOGI(TAG, "LittleFS mounted");

    // Ensure CSV exists, otherwise create with a sample row
    FILE *f = fopen(LFS_FILE_PATH, "r");
    if (!f) {
        ESP_LOGW(TAG, "CSV not found, creating default %s", LFS_FILE_PATH);
        f = fopen(LFS_FILE_PATH, "w");
        if (f) {
            fprintf(f, "Name,Flat_Number,MAC\n");
            fprintf(f, "Default Owner,A100,00:11:22:33:44:55\n");
            fclose(f);
        } else {
            ESP_LOGE(TAG, "Failed to create %s", LFS_FILE_PATH);
            return ESP_FAIL;
        }
    } else {
        fclose(f);
    }

    save_embedded_csv();
    load_owners_from_csv();
    return ESP_OK;
}

