================================================================
README - main10 (Changes vs main9)
================================================================

Summary: BIG UPDATE. Adds WiFi connectivity, replaces the old
software/NVS-based RTC with WiFi-based SNTP time sync, and adds a
GitHub uploader module so exported logs can be pushed to a GitHub
repository automatically.

Added files:
  - wifi_manager.c/.h : new module. Initializes WiFi in station mode,
    connects using credentials, exposes wifi_manager_init(),
    wifi_manager_wait_for_connection(), and wifi_manager_is_connected().
  - git_hub_uploader.c/.h : new module. Uses mbedTLS base64 encoding
    and esp_http_client to PUT a file's contents to a GitHub repo via
    the GitHub Contents API. github_upload_log() uploads the
    log_manager log file with a timestamped filename.

Key changes by file:

- rtc.c/.h: MAJOR REWRITE.
    * The entire old software-RTC implementation (manual
      increment_time()/day-in-month math, NVS save/load, 1-second
      tick task) is removed (kept commented out for reference) and
      replaced with a WiFi/SNTP-based RTC:
        - wifi_rtc_init(): sets IST timezone, starts SNTP, waits
          (with retries) for time sync.
        - rtc_is_time_valid(), rtc_get_time(), rtc_get_time_string(),
          rtc_resync() are new public APIs based on the system clock
          (time()/localtime_r()) rather than a custom struct.
    * rtc_get_time_str() (old name) is gone; call sites updated to
      rtc_get_time_string() (note: this rename isn't fully applied
      everywhere yet in this version - see lora_receive.c/activity_log.c
      which still reference the old name in some spots and the new
      name in others).

- activity_log.c/.h:
    * log_received_data() now calls rtc_get_time_string() instead of
      rtc_get_time_str().
    * clear_activity_logs() replaced with trim_old_logs(): instead of
      wiping the whole log file when MAX_LOGS is exceeded, it now
      keeps the most recent (MAX_LOGS - 1) entries by shifting the
      file contents, giving proper circular-buffer behavior.

- button_handler.c:
    * is_button_pressed() is now used consistently for BUTTON_1/2/
      BACK/UP/NEXT (previously some had double gpio_get_level checks
      inline); simplifies and de-duplicates the debounce logic.

- log_manager.c/.h:
    * copy_log_to_usb() gained (currently commented-out) LCD status
      message hooks in preparation for user feedback during export.

- lora_receive.c:
    * Calls to rtc_get_time_str() updated to rtc_get_time_string().

- main.c:
    * app_main() now initializes WiFi before anything else
      (wifi_manager_init(), wifi_manager_wait_for_connection()), and
      calls wifi_rtc_init() if connected, before mounting LittleFS/
      logging/system init.
    * usb_update_task priority raised (4 -> 10).

- password.c/.h:
    * load_pass_from_txt() signature simplified: no longer takes a
      path parameter, always reads from PASS_FILE_PATH.
    * Added load_pass_from_usb_to_flash(): extracted the
      USB->LittleFS password-file-copy logic (previously inline in
      usb_update.c) into password.c.

- screen_display.c/.h:
    * Exporting logs to USB now also attempts github_upload_log();
      all three (activity log copy, system log copy, GitHub upload)
      must succeed for "Log Exported" to be shown.
    * Added git_hub_uploader.h include.

- system_init.c/.h:
    * system_initialize() now calls load_owners_from_csv() and
      load_pass_from_txt() directly at startup (not just when a USB
      stick is inserted later), and app_rtc_init() call removed
      (replaced by WiFi-based RTC init in main.c).

- usb_update.c/.h:
    * load_owners_from_csv() signature simplified: no longer takes a
      path parameter (always uses LFS_FILE_PATH internally).
    * Password-file-copy logic extracted out to password.c (see
      above); usb_update.c now just calls
      load_pass_from_usb_to_flash().
    * Redundant "mount LittleFS first" check fully removed from
      usb_update_task() (mounting happens earlier in main.c now).

Net effect: the device is now WiFi-connected, has accurate
network-synced time, and can push exported logs directly to a
GitHub repository, in addition to the existing USB export path.
