================================================================
README - main9 (Changes vs main8)
================================================================

Summary: Adds a new "log_manager" module that captures all ESP_LOG
output into a file (and can export it to USB), re-enables the
silence button/buzzer functionality, and hardens the LoRa receive
task with mutex-protected LCD access.

Added files:
  - log_manager.c/.h : new module. Hooks into ESP-IDF's logging
    system via esp_log_set_vprintf() to write every log line (with a
    timestamp) to "/littlefs/log.txt", and provides
    copy_log_to_usb() to copy that file to a USB drive
    ("/usb/log.txt"). log_manager_init() overwrites the log file on
    every boot.

Key changes by file:

- activity_log.c/.h:
    * clear_activity_logs() renamed/relocated: header comment cleanup
      only (function itself carried over unchanged from main8 apart
      from removing a dead trim_old_logs() draft).

- button_handler.c/.h:
    * Silence button support is FULLY RE-ENABLED: Silence_BUTTON is
      back in the GPIO bit-mask, is_button_pressed() debounce helper
      is restored, and handle_silence_button() is restored.
    * button_task() again checks the silence button first
      (unconditionally) and only polls the other 5 buttons while
      !shouldBeep, matching the pre-main5 behavior but now built on
      top of the is_button_pressed() debounce helper introduced in
      main3/main7.
    * STATE_SEND_TO_USB renamed to STATE_SEND_ACTLOG_TO_USB throughout
      button handling.

- lcd_i2c.c:
    * Minor cleanup: removed now-unused `address` local variable in
      i2c_lcd_set_cursor() (dead code left over from main4).

- lora_receive.c/.h:
    * shouldBeep and buzzer_control_task() are FULLY RE-ENABLED
      (buzzer stack size increased to 4096).
    * lora_receive_task is rewritten as a new, more defensive
      implementation: safer null-termination, parses the sender via
      strchr() for '|' instead of strtok(), validates the sender
      isn't empty, and now protects LCD writes with lcd_mutex
      (xSemaphoreTake/Give) when displaying the recognized owner.
    * lora_receive_task stack size increased 4096 -> 8192.
    * The old (main7-era) lora_receive_task implementation is kept
      commented out for reference.

- main.c:
    * app_main() now calls mount_littlefs() and log_manager_init() at
      startup, before system_initialize(). receiver_start() call
      removed from main.c (moved into system_initialize(), see
      system_init.c below).

- password.h:
    * Renamed USB_PASS_FILE -> PASS_USB_FILE and
      SPIFFS_PASS_FILE -> PASS_FILE_PATH for clearer naming.

- screen_display.c/.h:
    * STATE_SEND_TO_USB renamed to STATE_SEND_ACTLOG_TO_USB.
    * Exporting the activity log to USB now also calls
      copy_log_to_usb() (from the new log_manager module) in addition
      to copy_activity_to_usb(), so both the activity log and the
      general system log get exported together.
    * The lcd_mutex take/give around show_screen() body was disabled
      (commented out) in this version - screen updates are no longer
      wrapped in the mutex here (mutex usage moved to be more
      localized, e.g. inside lora_receive.c).
    * display_task()'s old commented alternate implementation
      removed.

- system_init.c/.h:
    * system_initialize() now calls receiver_start() itself (moved
      from main.c), and includes log_manager.h.

- usb_update.c/.h:
    * mount_littlefs() changed from `static bool` to public
      `esp_err_t mount_littlefs(void)` so it can be called directly
      from main.c.
    * PASS_USB_FILE / PASS_FILE_PATH renaming applied here too.
    * The internal "mount LittleFS first" safety check inside
      usb_update_task() was commented out (now handled earlier in
      main.c).
