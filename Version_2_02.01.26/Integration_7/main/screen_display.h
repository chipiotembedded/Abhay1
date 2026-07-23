#ifndef SCREEN_DISPLAY_H
#define SCREEN_DISPLAY_H

#include "system_init.h"
#include "system_states.h"
#include <stddef.h>

void show_screen(StateID state);
void show_menu(const MenuItem *menu, size_t count);
void request_screen(StateID state);
#endif