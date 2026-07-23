================================================================
README - main11 (Changes vs main10)
================================================================

Summary: Adds activity-log GitHub upload, persists WiFi credentials
from a text file (instead of hardcoded), improves boot-time user
feedback on the LCD, adds a task watchdog, and reverts the
activity-log circular buffer back to a simple "wipe on full" scheme.

Key changes by file:

- activity_log.c/.h:
    * log_received_data() reworked: now checks total_logs >= MAX_LOGS
      up front and calls clear_activity_logs() (full wipe) BEFORE
      writing the new entry, instead of the trim_old_logs()
      (shift-and-keep-recent) approach introduced in main10. This is
      a revert to the simpler main9-era approach.
    * trim_old_logs() removed again; clear_activity_logs() restored.

- git_hub_uploader.c/.h:
    * Added github_upload_activity_log(): mirrors
      github_upload_log() but uploads the activity log file instead,
      with filename format "(Activity_Log)YYYY-MM-DD_HH:MM.txt".
      github_upload_log()'s filename format also updated to
      "(Log)YYYY-MM-DD_HH:MM.txt" (previously
      "YYYY-MM-DD_HH-MM-SS.txt").
    * Removed the temporary "Sending to Git" LCD message previously
      shown at the start of github_upload_log() (now handled by
      caller in screen_display.c instead).

- lcd_i2c.c:
    * lcd_backlight_timeout_task's inactivity timeout raised from
      10 seconds to 60 seconds.
    * i2c_lcd_clear() delay reduced from 10ms to 5ms.

- log_manager.c/.h:
    * log_manager_init() simplified: removed a dead alternate
      implementation, added a 100ms settle delay before hooking the
      custom logger.

- lora_receive.c/.h:
    * shouldBeep = true is re-enabled in the primary (non-commented)
      receive path when an owner is recognized (had been temporarily
      disabled).
    * Added esp_task_wdt.h include and prepared (mostly commented-out)
      task-watchdog registration hooks for the LoRa receive task.
    * Minor: removed the fixed tx power call
      (lora_set_tx_power(17)) from receiver_start().
    * lora_receive_task priority lowered from 24 to 10.

- main.c:
    * app_main() reworked with much more explicit step-by-step
      ESP_LOGI messages ("Step 1: Mounting LittleFS", etc.) and LCD
      status messages ("Plz Wait for Wifi and Time", "WiFi Connected"/
      "Wifi failed") shown via lcd_mutex-protected direct LCD calls.
    * Display/button/USB update task stack sizes increased (e.g.
      display task 4096->8192, button task 4096->8192).
    * Added (mostly commented-out) scaffolding for a task watchdog
      (esp_task_wdt) covering the main tasks.

- rtc.c/.h:
    * wifi_rtc_init() now returns bool (success/failure) instead of
      void.
    * rtc_wait_for_sync() now shows "Time Synced" / "Time failed" on
      the LCD (via lcd_mutex) in addition to logging.
    * Added rtc.h include of lcd_i2c.h (needed for the new LCD
      status messages).

- screen_display.c/.h:
    * request_screen() reworked to send the requested state through
      display_queue (xQueueSend) instead of calling show_screen()
      directly, restoring/strengthening the queue-based display_task
      architecture (with a fallback direct-call path if the queue
      isn't ready yet).
    * show_screen()'s top-level lcd_mutex guard removed (mutex usage
      now handled by the STATE_SEND_ACTLOG_TO_USB case and other
      call sites individually, to avoid deadlocking during long
      operations like GitHub upload).
    * STATE_SEND_ACTLOG_TO_USB now also calls
      github_upload_activity_log() (4 operations total: activity log
      USB copy, system log USB copy, log GitHub upload, activity log
      GitHub upload) and shows a combined "Export OK / Git: OK" or
      "failed" status.

- system_init.c/.h:
    * load_wifi_cred_from_txt() call re-enabled (was commented out in
      main10) - WiFi credentials are now actively loaded from a
      config file at startup instead of only via USB.

- usb_update.c:
    * load_wifi_cred_from_usb_to_flash() call re-enabled in
      copy_csv_to_lfs(), so WiFi credentials can also be updated via
      USB drive.

- wifi_manager.c/.h:
    * MAJOR REWORK: hardcoded WIFI_SSID/WIFI_PASSWORD macros removed;
      wifi_manager_init() now loads credentials from a text file
      (load_wifi_cred_from_txt(), reading into global ssid[]/
      password[] buffers) with a fallback SSID/password if no file is
      found.
    * Added load_wifi_cred_from_usb_to_flash() and
      load_wifi_cred_from_txt(): copy/read WiFi credentials from
      USB_WIFI_CRED_PATH / LFS_WIFI_CRED_PATH respectively.
    * WiFi event handler registration switched from
      esp_event_handler_instance_register() to the simpler
      esp_event_handler_register().
    * Added WiFi robustness settings: pmf_cfg (capable=true,
      required=false), failure_retry_cnt = 5, and disabling WiFi
      power-save mode (WIFI_PS_NONE) during time sync.
