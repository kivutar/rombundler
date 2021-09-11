typedef struct
{
	const char* title;
    const char* core;
    const char* rom;
} config;

int handler(void* user, const char* section, const char* name, const char* value);
