================================================================
README - main4 (Changes vs main3)
================================================================

Summary: Introduces an LCD mutex to make LCD access thread-safe
across tasks, moves screen updates onto a dedicated display_task
driven by a queue, and experiments with (but ultimately keeps
disabled) a queue-based LoRa receive redesign.

Key changes by file:

- lcd_i2c.c/.h:
    * Added a global `lcd_mutex` semaphore, created in i2c_lcd_init().
    * i2c_lcd_set_cursor() simplified (removed intermediate `address`
      variable, minor refactor); commented-out mutex-protected
      variants of set_cursor/send_string/clear left as reference for
      a future approach that wasn't adopted yet in this version.
    * i2c_lcd_clear() delay increased from 2ms to 10ms.

- button_handler.c:
    * POST_PRESS_DELAY_MS reduced from 200 to 130 for snappier
      response.

- lora_receive.c:
    * The active lora_receive_task/receiver_start implementation is
      unchanged in behavior, but a large alternative implementation
      (using a queue-based FlatOwner message system with
      xQueueOverwrite/lora_rx_queue instead of direct LCD writes) was
      drafted and left commented out at the bottom of the file for
      future reference. It is NOT active in this version.

- main.c:
    * Removed the local display_task() (moved to screen_display.c).
    * display_queue is now non-static (accessible from other files).
    * current_state global moved out of main.c (now lives in
      screen_display.c).

- screen_display.c/.h:
    * Added display_task(void *arg): a new FreeRTOS task that reads
      StateID values from display_queue and calls show_screen() while
      holding lcd_mutex, decoupling screen requests from screen
      rendering.
    * request_screen() simplified to always set current_state and
      call show_screen() directly (no longer bounds-checks state or
      skips redraw of the same state).
    * show_screen() now takes lcd_mutex for its entire body (guards
      against concurrent LCD writes from multiple tasks), and returns
      early with an error log if the mutex isn't initialized yet.
    * show_menu() reworked to always print "Menu" then up to 3 items
      on subsequent lines (previously cleared screen itself; now
      caller/show_screen owns clearing).
    * STATE_ACTIVITY_LOG now takes nvs_mutex with a 200ms timeout
      instead of blocking forever, improving robustness.
    * A speculative STATE_LORA_PACKET case (queue-driven LoRa display)
      was added but left commented out, alongside a matching
      (also commented out) STATE_LORA_PACKET enum entry.
    * A default case was added to show_screen() to display
      "Invalid State" for any unhandled state.
