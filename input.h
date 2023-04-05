#include "libretro.h"
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

uint16_t input_get_state(unsigned port);
void input_set_state(uint16_t state[]);
void input_poll(void);
int16_t input_state(unsigned port, unsigned device, unsigned index, unsigned id);
void input_set_keyboard_callback(retro_keyboard_event_t e);
