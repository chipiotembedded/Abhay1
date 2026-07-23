================================================================
README - main5 (Changes vs main4)
================================================================

Summary: Temporarily disables the silence button and buzzer
functionality (likely for debugging/testing hardware without a
buzzer wired up, or to isolate a bug).

Key changes by file:

- button_handler.c/.h:
    * Silence_BUTTON pin definition commented out.
    * handle_silence_button() function commented out.
    * The GPIO bit-mask configuration for buttons no longer includes
      Silence_BUTTON (left as a trailing comment).
    * button_task() no longer checks/handles the silence button at
      all; the "only poll other buttons if !shouldBeep" guard was
      also commented out, so buttons are now always polled
      regardless of shouldBeep state (since shouldBeep-related code
      was disabled elsewhere too).

- lora_receive.c/.h:
    * shouldBeep global variable declaration/definition commented out.
    * buzzer_control_task() function body commented out (buzzer no
      longer toggled based on shouldBeep).
    * BUZZER_PIN definition commented out in the header.
    * The buzzer_control_task is no longer started in receiver_start().
    * The line that set shouldBeep = true on a recognized owner is
      commented out.

Net effect: this version effectively disables the audible
buzzer/panic alert path while keeping the rest of the receive and
button logic intact, likely as an intermediate debugging step.
