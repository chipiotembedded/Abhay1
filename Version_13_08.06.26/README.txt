================================================================
README - main13 (Changes vs main12)
================================================================

Summary: Splits the combined "export all logs" menu option into a
proper submenu (separate activity-log vs system-log export), does a
major cleanup/refactor of the GitHub uploader (removing dead/legacy
WiFi-scan code and de-duplicating the upload logic into a shared
helper), and finishes enabling the LCD task's mutex protection.

Key changes by file:

- lcd_i2c.c:
    * lcd_task() now actually takes lcd_mutex around each LCD queue
      message it processes (previously left commented out in
      main12 - this was the last piece of the main12 LCD-queue
      rework, now fully enabled).

- button_handler.c/.h:
    * entered_password[], current_digit_index, and
      password_verified globals moved here from main.c (better
      encapsulation - button/password state now lives with the
      button handler code that uses it).
    * New navigation: STATE_LEVEL2_MENU's second button now leads to
      a new STATE_SEND_ALL_LOGS submenu (instead of going straight to
      export); from STATE_SEND_ALL_LOGS, button 1 goes to
      STATE_SEND_ACTIVITY_LOGS and button 2 goes to
      STATE_SEND_SYSTEM_LOGS. Back-button paths updated to match
      (STATE_SEND_ACTIVITY_LOGS / STATE_SEND_SYSTEM_LOGS both return
      to STATE_SEND_ALL_LOGS; STATE_SEND_ALL_LOGS returns to
      STATE_LEVEL2_MENU).

- git_hub_uploader.c/.h: MAJOR CLEANUP.
    * Removed a large block of legacy/duplicate WiFi init and
      scanning code (wifi_init_sta, wifi_scan_task, a local
      wifi_event_handler, and GitHub-time-sync-via-HTTP-Date-header
      logic) that had been sitting unused/duplicated in this file
      since earlier versions - WiFi and time sync are properly owned
      by wifi_manager.c/rtc.c elsewhere now.
    * github_upload_log() and github_upload_activity_log() were
      consolidated into a single shared static helper,
      github_upload_file(filepath, filename_fmt), which handles
      timestamped filename generation, file reading, and the actual
      upload; github_upload_log()/github_upload_activity_log() are
      now presumably thin wrappers around this helper (removing
      hundreds of lines of duplicated base64/HTTP logic).
    * GitHub token value updated/rotated.

- lora_receive.c/.h:
    * Added esp_task_wdt_reset() call at the end of each
      lora_receive_task loop iteration, and the task handle is now
      captured and registered with the Task Watchdog Timer
      (esp_task_wdt_add) in receiver_start(), improving crash/hang
      recovery.
    * Moved the esp_task_wdt.h include from the header into the .c
      file only.
    * Added "REG_VERSION=0x%02X OP_MODE=0x%02X IRQ=0x%02X", 
       at the beginng of while() for checking lora status every 5 sec
    * Added ESP_LOGI(TAG,"After lora_receive(): OP_MODE=0x%02X",lora_read_reg(REG_OP_MODE));
       after 2nd lora_receive to check lora status after packet reception.

- main.c:
    * app_main() reordered: LCD task creation now happens
      immediately after system_initialize() (with explicit
      "Step 4: LCD Task initialize" logging), and USB update task
      creation was moved to AFTER the WiFi connection step
      ("Step 8: USB Task initialize") rather than before it.
    * Several ESP_LOGI calls temporarily replaced with plain
      printf(TAG, ...) calls for some step-progress logs (note: this
      is technically a bug since printf doesn't take a tag argument
      the way ESP_LOGI does, but appears intentional/in-progress in
      this version).
    * entered_password/current_digit_index/password_verified globals
      removed from main.c (moved to button_handler.c).
    * A duplicate/older commented-out copy of app_main() was deleted.

- screen_display.c/.h:
    * STATE_SEND_ACTLOG_TO_USB replaced with three new states:
      STATE_SEND_ALL_LOGS (new submenu screen showing "Send Act Logs"
      / "Send Sys Logs" options), STATE_SEND_ACTIVITY_LOGS (exports +
      GitHub-uploads only the activity log), and
      STATE_SEND_SYSTEM_LOGS (exports + GitHub-uploads only the
      system log). Each shows separate "USB: OK/Failed" and
      "Git: Done/Failed" status lines instead of one combined
      success/failure message.
    * Removed button_handler.h include (no longer needed after
      moving password state to button_handler.c).

- system_init.c:
    * receiver_start() call restored here (was moved out to main.c in
      main12; now called from system_initialize() again).
