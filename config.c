#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"
#include "ini.h"

void cfg_defaults(config *c)
{
	c->title = "ROMBundler";
	c->shader = "default";
	c->filter = "nearest";
	c->fullscreen = false;
	c->scale = 3;
	c->swap_interval = 1;
	c->hide_cursor = false;
	c->map_analog_to_dpad = true;
}

int cfg_handler(void* user, const char* section, const char* name, const char* value)
{
	config* c = (config*)user;

	#define MATCH(s, n) strcmp(section, s) == 0 && strcmp(name, n) == 0
	if (MATCH("", "title"))
		c->title = strdup(value);
	else if (MATCH("", "core"))
		c->core = strdup(value);
	else if (MATCH("", "rom"))
		c->rom = strdup(value);
	else if (MATCH("", "shader"))
		c->shader = strdup(value);
	else if (MATCH("", "filter"))
		c->filter = strdup(value);
	else if (MATCH("", "swap_interval"))
		c->swap_interval = atoi(value);
	else if (MATCH("", "fullscreen"))
		c->fullscreen = strcmp(value, "true") == 0;
	else if (MATCH("", "scale"))
		c->scale = atoi(value);
	else if (MATCH("", "hide_cursor"))
		c->hide_cursor = strcmp(value, "true") == 0;
	else if (MATCH("", "map_analog_to_dpad"))
		c->map_analog_to_dpad = strcmp(value, "true") == 0;
	else
		return 0;
	return 1;
}
