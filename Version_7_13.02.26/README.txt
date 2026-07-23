================================================================
README - main7 (Changes vs main5)
================================================================

NOTE: There is no "main6" folder in this collection, so this
README documents the differences directly from main5 to main7.

Summary: Re-enables the silence button wiring (partially) and
introduces packet framing for LoRa messages (sender ID separated by
a '|' delimiter), improving parsing robustness.

Key changes by file:

- button_handler.c:
    * The is_button_pressed() debounce helper is commented back out
      (its usage was already removed in main5); the GPIO bit-mask
      comment for Silence_BUTTON was reformatted but the button
      remains excluded from the active mask.

- lora_receive.c:
    * Receive buffer size reduced from 256 to 100 bytes, and
      lora_receive_packet() now reserves 1 byte for the null
      terminator up front (`sizeof(buf)-1`) rather than checking
      after the fact.
    * NEW: incoming packets are now expected in the form
      "<sender>|<rest>" - the code tokenizes the buffer on '|' using
      strtok() and only uses the sender portion for owner lookup
      and duplicate-suppression, instead of treating the whole raw
      buffer as the MAC/identifier string.
    * "Unknown MAC" screen now displays the parsed `sender` value
      instead of the raw buffer.
    * Removed a large (very old, already-unused) alternative
      implementation block that had been kept commented out at the
      bottom of the file since earlier versions - fully deleted here.

Net effect: LoRa receive parsing is now delimiter-aware, laying the
groundwork for richer packet payloads in later versions.
