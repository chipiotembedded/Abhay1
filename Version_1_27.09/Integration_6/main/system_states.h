#ifndef SYSTEM_STATES_H
#define SYSTEM_STATES_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define PASSWORD_LENGTH 4
#define BUTTON_1 4
#define BUTTON_2 5
#define BUTTON_BACK 6
#define BUTTON_UP 7      
#define BUTTON_NEXT 17
#define Silence_BUTTON 18

// Screen states
typedef enum {
    STATE_BOOT,
    STATE_NAME_OF_SOCIETY,
    STATE_MAIN_MENU,
    STATE_LEVEL1_MENU,
    STATE_LEVEL2_MENU,
    STATE_LEVEL3_MENU,
    STATE_ACTIVITY_LOG,
    STATE_DETAILED_LOG,
    STATE_FAULT,
    STATE_ACKNOWLEDGEMENT,
    STATE_SHOW_ALARM,
    STATE_CUSTOMER_CARE,
    STATE_FLAT_OWNER,
    STATE_PASSWORD,
    STATE_CURRENT_PASSWORD,
    STATE_SET_PASSWORD,
    STATE_UPDATE_INFO,
    STATE_SYSTEM_INFO,
    STATE_INSTALLATION_MODE,
    STATE_COUNT  
} StateID;

// Menu item structure
typedef struct {
    StateID state;
    const char* name;
} MenuItem;

extern const MenuItem main_menu_items[];
extern const MenuItem level1_menu_items[];
extern const MenuItem level2_menu_items[];
extern const MenuItem level3_menu_items[];

extern StateID current_state;
extern int entered_password[PASSWORD_LENGTH];
extern int current_digit_index;
extern const int correct_password[PASSWORD_LENGTH];
extern bool password_verified;

#endif