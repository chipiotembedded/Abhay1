#ifndef LORA_RECEIVE_H
#define LORA_RECEIVE_H

#include <stdbool.h>
#include <stdint.h>

// ================== Hardware Pins ==================
#define BUZZER_PIN 9
//#define BUTTON_PIN 18

// ================== Flat Owner Info ==================
typedef struct {
    char name[32];
    char flat[16];
    char mac[20];
} FlatOwner;

// ================== Globals ==================
extern bool shouldBeep;

// ================== Functions ==================
void receiver_start(void);                   // Start LoRa + tasks
FlatOwner* find_owner_by_mac(const char *);  // Lookup CSV

#endif // LORA_RECEIVE_H
