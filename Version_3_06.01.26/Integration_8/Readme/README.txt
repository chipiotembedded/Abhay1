================================================================
README - main3 (Changes vs main2)
================================================================

Summary: Adds PIN/password protection to the menu system, moves
StateID/menu type definitions into screen_display.h (removing
system_states.h), and improves button debouncing.

Added files:
  - password.c/.h : new module. Loads a password from a text file
    (password.txt) copied from USB to LittleFS, and holds the
    correct_password array used to validate PIN entry.

Removed files:
  - system_states.h : StateID enum and MenuItem struct moved into
    screen_display.h instead.

Key changes by file:

- button_handler.c:
    * Replaced the old fixed-delay debounce (vTaskDelay after every
      action) with a helper is_button_pressed() that does a
      press-then-recheck debounce (50ms).
    * Removed the old generic handle_button_press(int button)
      dispatcher function (now unused/dead code).
    * Silence button is now checked first and unconditionally on
      every loop iteration; the other 5 buttons are only polled
      while shouldBeep is false, and only require a single press
      check (debounce handled by is_button_pressed()).
    * Back button now also calls reset_password_input() when
      leaving the password screen.

- lora.c:
    * Reverted lora_sleep() to use the proper
      MODE_LONG_RANGE_MODE|MODE_SLEEP register value instead of the
      raw 0x00 value used temporarily in main2.

- lora_receive.c/.h:
    * Header now pulls in most of its own includes directly (lora.h,
      lcd_i2c.h, activity_log.h, button_handler.h, rtc.h,
      screen_display.h) rather than relying on the .c file; also adds
      MAX_OWNERS and exposes the owners[] array via extern.
    * Removed leftover unused code from the .c file (now just
      includes lora_receive.h - most includes centralized in header).

- main.c:
    * Added password.h include.
    * correct_password[] constant array commented out (now supplied
      by password.c) but not yet fully wired up in this version.

- screen_display.c/.h:
    * StateID enum, MenuItem struct, and related extern menu arrays
      moved from system_states.h into screen_display.h.
    * .c file greatly simplified (most includes now come transitively
      via the header).

- system_init.h:
    * Removed system_states.h include (types now via screen_display.h
      and button_handler.h include chain); added button_handler.h.

- usb_update.c/.h:
    * copy_csv_to_lfs() (renamed conceptually) now also copies
      password.txt from USB to LittleFS in addition to test.csv.
    * usb_update_task now also calls load_pass_from_txt() after
      loading owners, both at startup and after a fresh USB copy.
    * USB_FILE_PATH changed from "/usb/ESP/test.csv" to
      "/usb/test.csv".
    * MAX_OWNERS definition moved out of this header (now lives in
      lora_receive.h).
