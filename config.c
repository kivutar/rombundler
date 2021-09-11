#include <string.h>
#include "config.h"
#include "ini.h"

int handler(void* user, const char* section, const char* name, const char* value)
{
    config* pconfig = (config*)user;

    #define MATCH(s, n) strcmp(section, s) == 0 && strcmp(name, n) == 0
    if (MATCH("", "title"))
        pconfig->title = strdup(value);
	else if (MATCH("", "core"))
        pconfig->core = strdup(value);
    else if (MATCH("", "rom"))
        pconfig->rom = strdup(value);
    else
        return 0;
    return 1;
}
