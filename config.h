typedef struct
{
	const char* title;
    const char* core;
    const char* rom;
    int swap_interval;
} config;

int handler(void* user, const char* section, const char* name, const char* value);
