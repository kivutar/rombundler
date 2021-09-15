#include <stdbool.h>

typedef struct
{
	const char* title;
    const char* core;
    const char* rom;
    int swap_interval;
    bool fullscreen;
    int scale;
    bool hide_cursor;
    bool map_analog_to_dpad;
} config;

int cfg_handler(void* user, const char* section, const char* name, const char* value);
