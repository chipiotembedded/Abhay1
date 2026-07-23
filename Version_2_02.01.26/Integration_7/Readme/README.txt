================================================================
README - main2 (Changes vs main1)
================================================================

Summary: Replaces the CSV-from-flash owner loading mechanism with
a USB mass-storage based update mechanism, and adds LCD backlight
power-saving.

Added files:
  - usb_update.c/.h : new module. Mounts a USB mass-storage device
    (via ESP USB Host + MSC), copies test.csv from the USB drive to
    LittleFS, and loads flat-owner records from it. Replaces
    flash_csv.c/.h.

Removed files:
  - flash_csv.c/.h  : superseded by usb_update.c/.h
  - test.csv        : no longer bundled; now expected on a USB drive

Key changes by file:

- lcd_i2c.c/.h:
    * Added an LCD backlight inactivity-timeout mechanism:
      lcd_register_activity() and lcd_backlight_timeout_task()
      turn the backlight off after 10 seconds of inactivity, and
      back on when a button/LoRa event occurs.

- button_handler.c:
    * Removed STATE_LEVEL3_MENU / installation-mode navigation
      branches (dead-end states not wired up yet).
    * Every button press now calls lcd_register_activity() to reset
      the backlight timeout.
    * Cleaned up several commented-out/dead code blocks.

- lora_receive.c:
    * Removed a large unused/duplicate commented-out implementation
      block at the top of the file (old "working code" reference).
    * Switched include from flash_csv.h to usb_update.h.
    * lora_receive_task now calls lcd_register_activity() on packet
      receipt, and its task priority was lowered (24 -> 10).
    * buzzer task's stack size reduced (4096 -> 1024).

- main.c:
    * Switched include from flash_csv.h to usb_update.h.
    * Removed old flash_csv mount/load calls.
    * Added creation of usb_update_task and lcd_backlight_timeout_task.

- rtc.c:
    * current_time year field changed from 2025 to 25 (2-digit).
    * rtc_get_time_str() format changed from
      "YYYY-MM-DD HH:MM:SS" to "DD-MM-YY HH:MM" (no seconds).
    * rtc_task stack size reduced (4096 -> 2048).

- screen_display.c:
    * Removed STATE_LEVEL3_MENU / STATE_UPDATE_INFO / 
      STATE_INSTALLATION_MODE menu handling (features not ready yet).
    * STATE_BOOT welcome animation reworked (shows "Welcome!" +
      "Plz Wait..." animation instead of "Welcome...").
    * STATE_FLAT_OWNER now checks total_owners > 0 before showing a
      slide, displaying "No owners found" otherwise.

- system_init.c:
    * Replaced flash_csv_init()/usb_update calls with a call to
      app_rtc_init() and proper NVS flash init sequence.

- system_states.h:
    * Removed a number of unused states (STATE_DETAILED_LOG,
      STATE_FAULT, STATE_ACKNOWLEDGEMENT, STATE_SHOW_ALARM,
      STATE_CUSTOMER_CARE, STATE_CURRENT_PASSWORD, STATE_SET_PASSWORD,
      STATE_SYSTEM_INFO) that were never implemented.
