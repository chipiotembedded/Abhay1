#include "usb_update.h"
static const char *TAG = "USB_UPDATE";
static bool littlefs_mounted=false;
// Global variables
FlatOwner owners[MAX_OWNERS];
int total_owners = 0;
int current_owner = 0;

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

// App event queue
static QueueHandle_t app_queue;
typedef struct {
    enum {
        APP_DEVICE_CONNECTED,
        APP_DEVICE_DISCONNECTED,
    } id;
    union {
        uint8_t new_dev_address;
    } data;
} app_message_t;

// --- Mount LittleFS ---
esp_err_t mount_littlefs(void) {
    if (littlefs_mounted) {
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Mounting LittleFS...");
    esp_vfs_littlefs_conf_t conf = {
        .base_path = "/littlefs",
        .partition_label = "storage",
        .format_if_mount_failed = true,
        .dont_mount = false,
    };
    
    esp_err_t ret = esp_vfs_littlefs_register(&conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to mount LittleFS: %s", esp_err_to_name(ret));
        return ret;
    }
    
    littlefs_mounted = true;
    ESP_LOGI(TAG, "LittleFS mounted successfully");
    return ESP_OK;
}

// --- MSC Event Callback ---
static void msc_event_cb(const msc_host_event_t *event, void *arg) {
    app_message_t msg;
    if (event->event == MSC_DEVICE_CONNECTED) {
        msg.id = APP_DEVICE_CONNECTED;
        msg.data.new_dev_address = event->device.address;
    } else {
        msg.id = APP_DEVICE_DISCONNECTED;
    }
    xQueueSend(app_queue, &msg, portMAX_DELAY);
}

// --- Copy CSV from USB to LittleFS ---
static bool copy_csv_to_lfs() {
    struct stat st;
    if (stat(USB_FILE_PATH, &st) != 0) {
        ESP_LOGE(TAG, "USB file not found: %s", USB_FILE_PATH);
        return false;
    }

    FILE *src = fopen(USB_FILE_PATH, "r");
    if (!src) {
        ESP_LOGE(TAG, "Failed to open USB file");
        return false;
    }

    FILE *dst = fopen(LFS_FILE_PATH, "w");
    if (!dst) {
        ESP_LOGE(TAG, "Failed to create LFS file");
        fclose(src);
        return false;
    }

    char buf[128];
    size_t r;
    while ((r = fread(buf, 1, sizeof(buf), src)) > 0) {
        fwrite(buf, 1, r, dst);
    }

    fclose(src);
    fclose(dst);
    ESP_LOGI(TAG, "CSV copied to LittleFS");

    load_pass_from_usb_to_flash();
    load_wifi_cred_from_usb_to_flash();

    return true;
}

// --- Load CSV into owners[] ---
void load_owners_from_csv(void) {
    FILE *f = fopen(LFS_FILE_PATH, "r");
    if (!f) {
        ESP_LOGE(TAG, "Failed to open %s", LFS_FILE_PATH);
        return;
    }

    char line[128];
    total_owners = 0;

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

            // ESP_LOGI(TAG, "Row %d: name='%s' flat='%s' mac='%s'",
            //          total_owners,
            //          owners[total_owners].name,
            //          owners[total_owners].flat,
            //          owners[total_owners].mac);

            total_owners++;
        }
    }
    fclose(f);
}

// --- Display one slide on LCD ---
void show_owner_slide(int index) {
    if (total_owners == 0 || index < 0 || index >= total_owners) {
        // i2c_lcd_clear();
        // i2c_lcd_set_cursor(0, 0);
        // i2c_lcd_send_string("No data available");
        lcd_display_text(0, 0, "No Data Available", true);
        return;
    }

    //i2c_lcd_clear();
    vTaskDelay(pdMS_TO_TICKS(10));

    char line[17];

    // Line 1: Name (truncate if longer than 16)
    snprintf(line, sizeof(line), "%.16s", owners[index].name);
    // i2c_lcd_set_cursor(0, 0);
    // i2c_lcd_send_string(line);
    lcd_display_text(0, 0, line, true);

    // Line 2: Flat
    snprintf(line, sizeof(line), "%.16s", owners[index].flat);
    // i2c_lcd_set_cursor(1, 0);
    // i2c_lcd_send_string(line);
    lcd_display_text(1, 0, line, false);

    // Line 3–4: MAC (split if longer than 16)
    char mac_part1[17] = {0};
    char mac_part2[17] = {0};

    strncpy(mac_part1, owners[index].mac, 16);
    mac_part1[16] = '\0';

    if (strlen(owners[index].mac) > 16) {
        strncpy(mac_part2, owners[index].mac + 16, 16);
        mac_part2[16] = '\0';
    }

    // i2c_lcd_set_cursor(2, 0);
    // i2c_lcd_send_string(mac_part1);

    // i2c_lcd_set_cursor(3, 0);
    // i2c_lcd_send_string(mac_part2);

    lcd_display_text(2, 0, mac_part1, false);
    lcd_display_text(3, 0, mac_part2, false);
}

// --- USB MSC Task ---
static void usb_task(void *args) {
    // Install USB host
    usb_host_install(&(usb_host_config_t){ 
        .intr_flags = ESP_INTR_FLAG_LEVEL1 
    });

    // Install MSC host driver
    msc_host_install(&(msc_host_driver_config_t){
        .create_backround_task = true,
        .task_priority = 5,
        .stack_size = 4096,
        .callback = msc_event_cb,
    });

    // Handle USB events
    uint32_t event_flags;
    while (1) {
        usb_host_lib_handle_events(portMAX_DELAY, &event_flags);
        if (event_flags & USB_HOST_LIB_EVENT_FLAGS_NO_CLIENTS) {
            if (usb_host_device_free_all() == ESP_OK) {
                break;
            }
        }
    }

    ESP_LOGI(TAG, "USB task cleanup");
    usb_host_uninstall();
    vTaskDelete(NULL);
}

// --- USB Update Task ---
void usb_update_task(void *arg) {
    ESP_LOGI(TAG, "USB update task started");

    // Create event queue
    app_queue = xQueueCreate(5, sizeof(app_message_t));

    // Start USB task
    xTaskCreate(usb_task, "usb_task", 4096, NULL, 2, NULL);

    // Try to load existing CSV from LittleFS
    // load_owners_from_csv();
    // load_pass_from_txt();
    // load_wifi_cred_from_txt();
    ESP_LOGI(TAG, "Waiting for USB device");

    msc_host_device_handle_t dev = NULL;
    msc_host_vfs_handle_t vfs = NULL;

    while (1) {
        app_message_t msg;
        xQueueReceive(app_queue, &msg, portMAX_DELAY);

        if (msg.id == APP_DEVICE_CONNECTED) {
            ESP_LOGI(TAG, "USB connected - installing device");

            // Install MSC device
            esp_err_t ret = msc_host_install_device(msg.data.new_dev_address, &dev);
            if (ret != ESP_OK) {
                ESP_LOGE(TAG, "Failed to install device: %s", esp_err_to_name(ret));
                continue;
            }

            // Register VFS
            esp_vfs_fat_mount_config_t config = {
                .format_if_mount_failed = false,
                .max_files = 3,
                .allocation_unit_size = 8192,
            };
            
            ret = msc_host_vfs_register(dev, "/usb", &config, &vfs);
            if (ret != ESP_OK) {
                ESP_LOGE(TAG, "Failed to register VFS: %s", esp_err_to_name(ret));
                msc_host_uninstall_device(dev);
                dev = NULL;
                continue;
            }

            // Wait before accessing files
            //vTaskDelay(pdMS_TO_TICKS(1000));

            // Copy and load CSV
            if (copy_csv_to_lfs()) {
                load_owners_from_csv();
                load_pass_from_txt();
                load_wifi_cred_from_txt();
            }
        }

        if (msg.id == APP_DEVICE_DISCONNECTED) {
            ESP_LOGI(TAG, "USB disconnected");
            if (vfs) {
                msc_host_vfs_unregister(vfs);
                vfs = NULL;
            }
            if (dev) {
                msc_host_uninstall_device(dev);
                dev = NULL;
            }
        }
    }
}
