#ifndef SCREEN_DISPLAY_H
#define SCREEN_DISPLAY_H

//#include "lcd_i2c.h"
//#include "button_handler.h"
#include "system_init.h"
#include "system_states.h"
#include <stddef.h>

void show_screen(StateID state);
void show_menu(const MenuItem *menu, size_t count);
//void show_password_screen(uint8_t position, int *password);
void request_screen(StateID state);
#endif