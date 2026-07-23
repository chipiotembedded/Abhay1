================================================================
README - main8 (Changes vs main7)
================================================================

Summary: Replaces NVS-based activity log storage with a flat file
on LittleFS, and adds the ability to export the activity log to a
USB drive via a new menu option.

Key changes by file:

- activity_log.c/.h:
    * MAJOR CHANGE: activity logging no longer uses NVS
      (nvs_flash/blob storage with a mutex). Logs are now stored as
      raw log_entry structs appended to a file at
      ACTIVITY_FILE_PATH ("/littlefs/activity.txt").
    * initialize_activity_log() no longer initializes NVS; it just
      reads the current total_logs from the file's size.
    * log_received_data() now fopen()'s the file in append mode and
      fwrite()'s a log_entry struct directly, instead of writing to
      an NVS blob. If total_logs exceeds MAX_LOGS, the log file is
      cleared and the counter reset to 1 (simple circular-buffer
      stand-in).
    * get_total_logs() now computes total logs from file size /
      sizeof(log_entry) instead of an NVS counter.
    * read_log_entry() now seeks to the correct offset in the file
      and reads the struct directly, instead of doing NVS blob
      lookups with circular-buffer index math.
    * Added clear_activity_logs(): truncates/overwrites the activity
      log file.
    * Added copy_activity_to_usb(): reads all log_entry records from
      the LittleFS activity file and writes them out as
      comma-separated text ("Name,Flat,Timestamp") to a file on the
      USB drive (ACTIVITY_USB_FILE_PATH).
    * nvs_mutex and NVS_NAMESPACE removed (no longer needed).

- button_handler.c:
    * Added a new navigation path: from STATE_LEVEL2_MENU, pressing
      button 2 now goes to the new STATE_SEND_TO_USB screen; from
      STATE_SEND_TO_USB, the back button returns to STATE_LEVEL2_MENU.

- screen_display.c/.h:
    * Added a new menu entry "Send ActLog" (STATE_SEND_TO_USB) to
      level2_menu_items.
    * STATE_ACTIVITY_LOG now calls get_total_logs() directly (file
      size based) instead of going through the old NVS mutex/logic.
    * Added handling for the new STATE_SEND_TO_USB state: calls
      copy_activity_to_usb() and shows "Log Exported" or
      "Export Failed" on the LCD accordingly.
    * Added new STATE_SEND_TO_USB value to the StateID enum
      (replacing the earlier speculative/commented STATE_LORA_PACKET
      placeholder).

- usb_update.c:
    * Minor: verbose per-row CSV parse logging commented out to
      reduce log noise.
