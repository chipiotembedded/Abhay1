================================================================
README - main1 (Baseline Version)
================================================================

This is the earliest available version of the project in this
collection. It establishes the core architecture of the system:

- ESP32 firmware for a LoRa-based "flat owner" visitor/entry
  system with an I2C LCD display, physical buttons, buzzer,
  activity logging, and a simple software RTC.

Key modules present in this version:
  - main.c              : app entry point, task creation
  - system_init.c/.h     : central hardware/system initialization
  - system_states.h      : screen/menu state machine definitions
  - screen_display.c/.h  : LCD screen rendering, menus
  - button_handler.c/.h  : physical button input handling
  - lcd_i2c.c/.h         : low-level I2C LCD driver
  - lora.c/.h            : low-level LoRa radio driver
  - lora_receive.c/.h    : LoRa packet receive task, owner lookup,
                           panic/buzzer handling
  - activity_log.c/.h    : NVS-based activity logging
  - rtc.c/.h             : simple software RTC (manual time keeping,
                           saved/loaded from NVS)
  - flash_csv.c/.h       : loads flat-owner records from a CSV file
                           stored on flash (test.csv included)

Notable characteristics of this baseline:
  - Owner data is loaded via "flash_csv" (test.csv), not yet via USB.
  - No password/PIN-protected menu yet.
  - No USB mass-storage update mechanism yet.
  - No WiFi, GitHub upload, or external log manager.
  - system_states.h defines a large number of states (many of which
    are unused later or later removed), including alarm/fault/
    customer-care states not used elsewhere.

This version serves as the starting point for all later changes.
