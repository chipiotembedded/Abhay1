#include "wifi_manager.h"

static const char *TAG = "WIFI_MANAGER";

// WIFI CREDENTIALS //
// #define WIFI_SSID      "Paras 2.4"
// #define WIFI_PASSWORD  "9890090011"

char ssid[32];
char password[64];

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1
#define MAX_RETRY          5

static EventGroupHandle_t s_wifi_event_group;
static int s_retry_num = 0;
static bool s_is_connected = false;


// Event Handler             //

static void wifi_event_handler(void *arg,
                               esp_event_base_t event_base,
                               int32_t event_id,
                               void *event_data)
{
    if (event_base == WIFI_EVENT &&
        event_id == WIFI_EVENT_STA_START)
    {
        ESP_LOGI(TAG, "WiFi Started. Connecting...");
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT &&
             event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        if (s_retry_num < MAX_RETRY)
        {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGW(TAG, "Retrying connection (%d/%d)",
                     s_retry_num, MAX_RETRY);
        }
        else
        {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
            ESP_LOGE(TAG, "Failed to connect to WiFi");

            // i2c_lcd_clear();
            // i2c_lcd_set_cursor(0,0);
            // i2c_lcd_send_string("WiFi failed");
        }
        s_is_connected = false;
    }
    else if (event_base == IP_EVENT &&
             event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));

        s_retry_num = 0;
        s_is_connected = true;
        
        //vTaskDelay(pdMS_TO_TICKS(2000));

        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}


// Public Functions          //
/*
esp_err_t wifi_manager_init(void)
{
    esp_err_t ret;

    // Initialize NVS //
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Create event group //
    s_wifi_event_group = xEventGroupCreate();

    // Initialize TCP/IP stack //
    ESP_ERROR_CHECK(esp_netif_init());

    // Create default event loop //
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Create default WiFi station //
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    static esp_event_handler_instance_t instance_any_id;
    static esp_event_handler_instance_t instance_got_ip;

    // Register event handlers //
    ESP_ERROR_CHECK(
        esp_event_handler_instance_register(
            WIFI_EVENT,
            ESP_EVENT_ANY_ID,
            &wifi_event_handler,
            NULL,
            &instance_any_id));

    ESP_ERROR_CHECK(
        esp_event_handler_instance_register(
            IP_EVENT,
            IP_EVENT_STA_GOT_IP,
            &wifi_event_handler,
            NULL,
             &instance_got_ip));

    // wifi_config_t wifi_config = {
    //     .sta = {
    //         .ssid = WIFI_SSID,
    //         .password = WIFI_PASSWORD,
    //         .threshold.authmode = WIFI_AUTH_WPA2_PSK,
    //     },
    // };

    // Load credentials first    
    wifi_config_t wifi_config = {0};

    if (load_wifi_cred_from_txt() == ESP_OK) {

        strlcpy((char *)wifi_config.sta.ssid,
                ssid,
                sizeof(wifi_config.sta.ssid));

        strlcpy((char *)wifi_config.sta.password,
                password,
                sizeof(wifi_config.sta.password));

    }
    else {
        ESP_LOGW(TAG, "Using fallback credentials");

        strlcpy((char *)wifi_config.sta.ssid,
                "FallbackSSID",
                sizeof(wifi_config.sta.ssid));

        strlcpy((char *)wifi_config.sta.password,
                "FallbackPASS",
                sizeof(wifi_config.sta.password));
    }
    
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;

    wifi_config.sta.pmf_cfg.capable = true;
    wifi_config.sta.pmf_cfg.required = false;
    //wifi_config.sta.failure_retry_cnt = 5;
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "WiFi initialization finished.");

    return ESP_OK;
}
*/

esp_err_t wifi_manager_init(void)
{
    esp_err_t ret;

    /* -------------------- NVS -------------------- */
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    /* -------------------- Event Group -------------------- */
    s_wifi_event_group = xEventGroupCreate();
    if (s_wifi_event_group == NULL) {
        ESP_LOGE(TAG, "Failed to create event group");
        return ESP_FAIL;
    }

    /* -------------------- Network Stack -------------------- */
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    /* -------------------- WiFi Init -------------------- */
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    /* -------------------- Event Handlers -------------------- */
    ESP_ERROR_CHECK(
        esp_event_handler_register(
            WIFI_EVENT,
            ESP_EVENT_ANY_ID,
            &wifi_event_handler,
            NULL));

    ESP_ERROR_CHECK(
        esp_event_handler_register(
            IP_EVENT,
            IP_EVENT_STA_GOT_IP,
            &wifi_event_handler,
            NULL));

    /* -------------------- Load Credentials -------------------- */
    wifi_config_t wifi_config = {0};

    if (load_wifi_cred_from_txt() == ESP_OK)
    {
        strlcpy((char *)wifi_config.sta.ssid,
                ssid,
                sizeof(wifi_config.sta.ssid));

        strlcpy((char *)wifi_config.sta.password,
                password,
                sizeof(wifi_config.sta.password));

        ESP_LOGI(TAG, "Using stored WiFi credentials");
    }
    else
    {
        ESP_LOGW(TAG, "No stored credentials, using fallback");

        strlcpy((char *)wifi_config.sta.ssid,
                "FallbackSSID",
                sizeof(wifi_config.sta.ssid));

        strlcpy((char *)wifi_config.sta.password,
                "FallbackPASS",
                sizeof(wifi_config.sta.password));
    }

    /* -------------------- WiFi Security & Retry -------------------- */
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    wifi_config.sta.pmf_cfg.capable = true;
    wifi_config.sta.pmf_cfg.required = false;
    wifi_config.sta.failure_retry_cnt = 5;

    /* -------------------- Start WiFi -------------------- */
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));  // Disable power save during sync
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "WiFi initialization finished");

    return ESP_OK;
}


/* Wait until connected or failed */
void wifi_manager_wait_for_connection(void)
{
    EventBits_t bits = xEventGroupWaitBits(
        s_wifi_event_group,
        WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
        pdFALSE,
        pdFALSE,
        portMAX_DELAY);

    if (bits & WIFI_CONNECTED_BIT)
    {
        ESP_LOGI(TAG, "Connected to WiFi successfully");
        // i2c_lcd_clear();
        // i2c_lcd_set_cursor(0,0);
        // i2c_lcd_send_string("WiFi Connected");
    }
    else if (bits & WIFI_FAIL_BIT)
    {
        ESP_LOGE(TAG, "WiFi connection failed");
        // i2c_lcd_clear();
        // i2c_lcd_set_cursor(0,0);
        // i2c_lcd_send_string("Wifi failed");
    }
}

/* Check connection status */
bool wifi_manager_is_connected(void)
{
    return s_is_connected;
}

esp_err_t load_wifi_cred_from_usb_to_flash(void){
    //For Wifi credentials
    FILE *src2 = fopen(USB_WIFI_CRED_PATH, "r");
    if (!src2) {
        ESP_LOGE(TAG, "Failed to open Wifi_credentials USB file");
        return false;
    }

    FILE *dst2 = fopen(LFS_WIFI_CRED_PATH, "w");
    if (!dst2) {
        ESP_LOGE(TAG, "Failed to create wifi_credentials LFS file");
        fclose(src2);
        return false;
    }

    char buf2[128];
    size_t r2;
    while ((r2 = fread(buf2, 1, sizeof(buf2), src2)) > 0) {
        fwrite(buf2, 1, r2, dst2);
    }

    fclose(src2);
    fclose(dst2);
    ESP_LOGI(TAG, "wifi cred txt copied to LittleFS");
    
    return ESP_OK;
}

esp_err_t load_wifi_cred_from_txt(void){
    FILE *f = fopen(LFS_WIFI_CRED_PATH, "r");
    if (!f) {
        ESP_LOGE(TAG, "Failed to open %s", LFS_WIFI_CRED_PATH);
        return ESP_FAIL;
    }

    // Read SSID
    if (fgets(ssid, MAX_SSID_LEN, f) == NULL) {
        ESP_LOGE(TAG, "Failed to read SSID");
        fclose(f);
        return ESP_FAIL;
    }

    // Remove newline
    ssid[strcspn(ssid, "\r\n")] = 0;

    // Read PASSWORD
    if (fgets(password, MAX_PASS_LEN, f) == NULL) {
        ESP_LOGE(TAG, "Failed to read password");
        fclose(f);
        return ESP_FAIL;
    }

    // Remove newline
    password[strcspn(password, "\r\n")] = 0;

    fclose(f);

    ESP_LOGI(TAG, "WiFi credentials loaded");
    ESP_LOGI(TAG, "SSID: %s", ssid);
    ESP_LOGI(TAG, "PASS: %s", password);
    
    vTaskDelay(pdMS_TO_TICKS(1000));
    return ESP_OK;
}

// void wifi_monitor_task(void *pv)
// {
//     wifi_manager_wait_for_connection();
//     wifi_rtc_init();
//     // if (wifi_manager_is_connected()) {
//     //     wifi_rtc_init();
//     // }

//     vTaskDelete(NULL);
// }


// void wifi_auto_connect_task(void *pv)
// {
//     if (load_wifi_cred_from_txt() == ESP_OK)
//     {
//         wifi_manager_wait_for_connection();   // start connection attempt

//         if (wifi_manager_is_connected()) {
//             wifi_rtc_init();
//             ESP_LOGI("WIFI", "Auto connect success");
//         } else {
//             ESP_LOGW("WIFI", "Auto connect failed");
//         }
//     }
//     else
//     {
//         ESP_LOGW("WIFI", "No stored credentials");
//     }

//     vTaskDelete(NULL);
// }
