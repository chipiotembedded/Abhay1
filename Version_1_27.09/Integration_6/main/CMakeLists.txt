idf_component_register(SRCS "lora_receive.c" "rtc.c" "button_handler.c" "flash_csv.c" "screen_display.c" "main.c" "system_init.c" "activity_log.c" "lcd_i2c.c" "lora.c"
                    INCLUDE_DIRS "."
                    EMBED_FILES "test.csv")