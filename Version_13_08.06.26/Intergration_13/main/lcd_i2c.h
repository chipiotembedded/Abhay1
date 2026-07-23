#ifndef I2C_LCD_H
#define I2C_LCD_H

#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "driver/i2c.h"
#include "esp_task_wdt.h"
#include <string.h>

// I2C Configuration
#define I2C_MASTER_SCL_IO          11        // GPIO number for I2C clock
#define I2C_MASTER_SDA_IO          12        // GPIO number for I2C data
#define I2C_MASTER_NUM             I2C_NUM_0 // I2C port number
#define I2C_MASTER_FREQ_HZ         100000    // I2C master clock frequency
#define I2C_LCD_ADDRESS            0x27      // LCD I2C address (may be 0x3F)
#define LCD_ROWS 4
#define LCD_COLS 16

// LCD Commands
#define LCD_CLEAR_DISPLAY          0x01
#define LCD_RETURN_HOME            0x02
#define LCD_ENTRY_MODE_SET         0x04
#define LCD_DISPLAY_CONTROL        0x08
#define LCD_FUNCTION_SET           0x20
#define LCD_SET_DDRAM_ADDR         0x80

// Flags for display entry mode
#define LCD_ENTRY_RIGHT            0x00
#define LCD_ENTRY_LEFT             0x02
#define LCD_ENTRY_SHIFT_INCREMENT  0x01
#define LCD_ENTRY_SHIFT_DECREMENT  0x00

// Flags for display control
#define LCD_DISPLAY_ON            0x04
#define LCD_DISPLAY_OFF           0x00
#define LCD_CURSOR_ON             0x02
#define LCD_CURSOR_OFF            0x00
#define LCD_BLINK_ON              0x01
#define LCD_BLINK_OFF             0x00

// Flags for function set
#define LCD_4BIT_MODE             0x00
#define LCD_2LINE                 0x08
#define LCD_5x8DOTS               0x00

// Backlight control
#define LCD_BACKLIGHT             0x08
#define LCD_NOBACKLIGHT           0x00

typedef struct {
    uint8_t line;      // 0-3
    uint8_t col;       // 0-15
    char text[17];     // Max 16 chars + null
    bool clear_first;  // Clear screen before writing
} lcd_message_t;

// Queue handle
extern QueueHandle_t lcd_queue;
extern SemaphoreHandle_t lcd_mutex;

// Function prototypes
void i2c_lcd_init(void);
void i2c_lcd_send_cmd(uint8_t cmd);
void i2c_lcd_send_data(uint8_t data);
void i2c_lcd_send_string(const char *str);
void i2c_lcd_set_cursor(uint8_t row, uint8_t col);
void i2c_lcd_clear(void);
void i2c_lcd_backlight(uint8_t state);
void i2c_lcd_write_char(char c);

void lcd_register_activity(void);
void lcd_backlight_timeout_task(void *arg);

// Function to send message to LCD task
void lcd_display_text(uint8_t line, uint8_t col, const char *text, bool clear_first);
void lcd_task(void *pvParameters);
#endif