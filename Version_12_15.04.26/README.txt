================================================================
README - main12 (Changes vs main11)
================================================================

Summary: MAJOR LCD ARCHITECTURE CHANGE. Replaces the mutex-guarded
direct-LCD-write model (and the display_queue/display_task screen
architecture) with a single dedicated LCD task driven by a message
queue (lcd_task + lcd_display_text()), used consistently across the
whole codebase. Also hardens the GitHub uploader against malloc/
read failures and improves RTC sync robustness with an SNTP
callback.

Key changes by file:

- lcd_i2c.c/.h: MAJOR REWORK.
    * Added lcd_message_t struct (line, col, text[17], clear_first)
      and a new global `lcd_queue`.
    * Added lcd_display_text(line, col, text, clear_first): the new
      standard way to update the LCD from any task - just posts a
      message to lcd_queue instead of taking a mutex and calling
      i2c_lcd_* functions directly.
    * Added lcd_task(): a dedicated FreeRTOS task that dequeues
      lcd_message_t messages and performs the actual
      i2c_lcd_clear/set_cursor/send_string calls, serializing all LCD
      access through a single task (removing the need for callers to
      manage lcd_mutex themselves in most places).
    * i2c_lcd_backlight() reworked to support being driven through
      the queue as well (via an is_backlight_cmd flag on the message,
      though direct i2c_send_byte is also still used); a
      static lcd_backlight_state tracks current backlight bits.
    * lcd_backlight_timeout_task/lcd_register_activity now take
      lcd_mutex around the underlying i2c_lcd_backlight() call.
    * i2c_lcd_set_cursor() now has an added 25ms delay before sending
      the cursor command (extra settle time for flaky LCD backpacks).
    * Various earlier commented-out mutex-based variants of
      set_cursor/send_string/clear were finally deleted.

- activity_log.c:
    * display_log_entry() switched from direct i2c_lcd_set_cursor/
      send_string calls to lcd_display_text(), matching the new LCD
      architecture.

- button_handler.c:
    * All request_screen() calls replaced with direct show_screen()
      calls (see screen_display.c below - the queue/task screen
      model was removed in favor of the new LCD-queue model).
    * "Wrong Password" message now shown via lcd_display_text()
      instead of manual mutex+i2c_lcd_* calls.
    * DEBOUNCE_DELAY_MS reverted from 40 back to 50.

- git_hub_uploader.c:
    * Both github_upload_log() and github_upload_activity_log() now
      check malloc() return values and verify fread() read the full
      expected number of bytes before proceeding, returning
      ESP_ERR_NO_MEM / ESP_FAIL on failure instead of risking a NULL
      dereference or partial/corrupt upload.
    * Filename timestamp format changed from "%Y-%m-%d_%H:%M" to
      "%d-%m-%Y_%H:%M" (day-month-year instead of year-month-day) for
      both log and activity-log uploads.

- lora_receive.c:
    * LCD writes for "owner found" / "unknown MAC" switched from
      direct i2c_lcd_* + lcd_mutex calls to lcd_display_text().
    * Receive-loop poll delay reduced from 100ms to 10ms.

- main.c:
    * app_main() restructured around the new LCD task: creates
      lcd_queue and starts lcd_task BEFORE other tasks, uses
      lcd_display_text() for all boot-time status messages ("Plz Wait
      for Wifi and Time", "Wifi Connected"/"Wifi Failed").
    * display_queue/display_task no longer created (removed - see
      screen_display.c).
    * Initial screen sequence now calls show_screen() directly
      instead of request_screen().
    * receiver_start() call moved into app_main() directly (previously
      inside system_initialize()).

- rtc.c/.h:
    * wifi_rtc_init() return type kept as bool; rtc_wait_for_sync()
      reworked to use an SNTP time-sync notification callback
      (rtc_time_sync_notification_cb) plus esp_sntp_get_sync_status()
      rather than purely polling localtime() in a retry loop, giving
      faster/more reliable sync detection. Added an explicit sync
      timeout (45s) with periodic progress logging.
    * If SNTP sync times out but the system clock is already valid
      (e.g. from a previous sync), rtc_wait_for_sync() now accepts
      that instead of reporting failure.
    * rtc_get_time_string() now returns "Time not set" if the clock
      isn't valid, instead of formatting garbage/epoch time.
    * All RTC status messages ("Time Synced", "Time Failed", "Time:")
      now go through lcd_display_text() instead of manual mutex
      handling.

- screen_display.c/.h: MAJOR CHANGE.
    * display_queue/display_task/request_screen()/show_menu()
      infrastructure REMOVED (commented out) - screens are now shown
      via direct show_screen() calls from button handlers and main.c.
    * Every i2c_lcd_set_cursor()/i2c_lcd_send_string() call in
      show_screen() replaced with lcd_display_text().
    * Menu screens (main menu, level1/level2 menus) no longer use the
      MenuItem array + show_menu() helper; menu text is now
      hardcoded directly in show_screen() for each state (simpler,
      but less data-driven).
    * "Send ActLog to USB" export status messages simplified/reworded
      ("Export OK  Git: OK" / "Failed").

- system_init.c:
    * Reordered initialization: i2c_lcd_init() now happens first,
      then owners/password/WiFi-credential loading, then buttons and
      activity log init. receiver_start() call removed here (moved to
      main.c).

- usb_update.c:
    * show_owner_slide() and the "no data available" message switched
      to lcd_display_text() calls instead of direct i2c_lcd_* calls.

- wifi_manager.c:
    * Minor: removed a 2-second delay after a successful WiFi connect
      event.
