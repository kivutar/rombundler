#include <stdbool.h>

typedef struct
{
	const char* title;
    const char* core;
    const char* rom;
    const char* shader;
    const char* filter;
    int swap_interval;
    bool fullscreen;
    int window_width;
    int window_height;
    float aspect_ratio;
    bool hide_cursor;
    bool map_analog_to_dpad;
} config;

void cfg_defaults(config* c);
int cfg_handler(void* user, const char* section, const char* name, const char* value);
