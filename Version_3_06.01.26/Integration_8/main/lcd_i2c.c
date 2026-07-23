#include "lcd_i2c.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include "esp_err.h"

static const char *TAG = "LCD_I2C";

// Improved I2C send function with error handling
static esp_err_t i2c_send_byte(uint8_t data) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (I2C_LCD_ADDRESS << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, data, true);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C send failed: %s", esp_err_to_name(ret));
    }
    return ret;
}

// Optimized nibble sending with proper timing
static void i2c_lcd_send_nibble(uint8_t nibble, uint8_t rs) {
    uint8_t data = (nibble << 4) | (rs ? 0x01 : 0x00) | LCD_BACKLIGHT;
    
    // Pulse the enable pin
    i2c_send_byte(data | 0x04);  // EN high
    esp_rom_delay_us(1);         // >450ns pulse width
    i2c_send_byte(data & ~0x04); // EN low
    esp_rom_delay_us(50);        // >37µs delay
}

// Initialize I2C LCD with robust sequence
void i2c_lcd_init(void) {
    // Configure I2C
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };
    ESP_ERROR_CHECK(i2c_param_config(I2C_MASTER_NUM, &conf));
    ESP_ERROR_CHECK(i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0));

    // Extended power-on delay (>40ms)
    vTaskDelay(pdMS_TO_TICKS(50));

    // Initialization sequence for 4-bit mode
    for (int i = 0; i < 3; i++) {
        i2c_lcd_send_nibble(0x03, 0);
        vTaskDelay(pdMS_TO_TICKS(i == 0 ? 5 : 1)); // First delay longer
    }
    i2c_lcd_send_nibble(0x02, 0); // Switch to 4-bit mode
    
    // Function set: 4-bit, 2-line, 5x8
    i2c_lcd_send_cmd(0x28);
    // Display on, cursor off, blink off
    i2c_lcd_send_cmd(0x0C);
    // Clear display
    i2c_lcd_send_cmd(0x01);
    vTaskDelay(pdMS_TO_TICKS(2));
    // Entry mode set
    i2c_lcd_send_cmd(0x06);
    
    vTaskDelay(pdMS_TO_TICKS(2));
}

// Send command to LCD
void i2c_lcd_send_cmd(uint8_t cmd) {
    i2c_lcd_send_nibble(cmd >> 4, 0);   // High nibble
    i2c_lcd_send_nibble(cmd & 0x0F, 0); // Low nibble
    
    // Special delays for clear and home commands
    if (cmd == 0x01 || cmd == 0x02) {
        vTaskDelay(pdMS_TO_TICKS(2));
    }
}

// Send data to LCD
void i2c_lcd_send_data(uint8_t data) {
    i2c_lcd_send_nibble(data >> 4, 1);   // High nibble
    i2c_lcd_send_nibble(data & 0x0F, 1); // Low nibble
}

// Set cursor position (16x4 LCD)
void i2c_lcd_set_cursor(uint8_t row, uint8_t col) {
    const uint8_t row_offsets[] = {0x00, 0x40, 0x10, 0x50};
    uint8_t address = col + (row < 4 ? row_offsets[row] : 0);
    i2c_lcd_send_cmd(0x80 | address);
}

// void i2c_lcd_set_cursor(uint8_t row, uint8_t col) {
//     uint8_t row_offsets[] = {0x00, 0x40, 0x10, 0x50};
//     if (row > 3) row = 3;  // limit row
//     i2c_lcd_send_cmd(0x80 | (col + row_offsets[row]));
// }

// Print string
void i2c_lcd_send_string(const char *str) {
    while (*str) {
        i2c_lcd_send_data(*str++);
    }
}

// Clear display
void i2c_lcd_clear(void) {
    i2c_lcd_send_cmd(0x01);
    vTaskDelay(pdMS_TO_TICKS(2));
}

// Control backlight
void i2c_lcd_backlight(uint8_t state) {
    uint8_t data = state ? LCD_BACKLIGHT : LCD_NOBACKLIGHT;
    i2c_send_byte(data);
}

void i2c_lcd_write_char(char c) {
    i2c_lcd_send_data((uint8_t)c);
}


///Backlight On-Off/////
// void lcd_backlight_on(void) {
//     i2c_lcd_backlight(1);
//     if (lcd_timer) {
//         xTimerReset(lcd_timer, 0);  // restart timer
//     }
// }

// void lcd_backlight_off_cb(TimerHandle_t xTimer) {
//     i2c_lcd_backlight(0);
// }

volatile uint32_t lcd_last_activity = 0;
bool lcd_backlight_is_on = true;

#define LCD_INACTIVITY_TIMEOUT_MS  10000   // 10 seconds

void lcd_backlight_timeout_task(void *arg)
{
    lcd_last_activity = xTaskGetTickCount();

    while (1)
    {
        uint32_t now = xTaskGetTickCount();

        if (lcd_backlight_is_on &&
            (now - lcd_last_activity) * portTICK_PERIOD_MS >= LCD_INACTIVITY_TIMEOUT_MS)
        {
            i2c_lcd_backlight(0);     // turn OFF
            lcd_backlight_is_on = false;
        }

        vTaskDelay(pdMS_TO_TICKS(500));  // check every 0.5 sec
    }
}

void lcd_register_activity(void)
{
    lcd_last_activity = xTaskGetTickCount();

    if (!lcd_backlight_is_on) {
        i2c_lcd_backlight(1);     // turn ON
        lcd_backlight_is_on = true;
    }
}

