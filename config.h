#include <stdbool.h>

typedef struct
{
	const char* title;
    const char* core;
    const char* rom;
    int swap_interval;
    int fullscreen;
    int scale;
    bool hide_cursor;
} config;

int handler(void* user, const char* section, const char* name, const char* value);
