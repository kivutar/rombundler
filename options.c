#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "options.h"

#define MAX_OPTIONS 64
#define MAX_STRING_SIZE 64
static int num_opts = 0;

char opt_keys[MAX_OPTIONS][MAX_STRING_SIZE];
char opt_values[MAX_OPTIONS][MAX_STRING_SIZE];

bool get_option(char const * key, char const ** value)
{
	int i;
	for (i = 0; i <= num_opts; i++)
	{
		if (strcmp(opt_keys[i], key) == 0)
		{
			*value = opt_values[i];
			return true;
		}
	}
	return false;
}

int opt_handler(void* user, const char* section, const char* name, const char* value)
{
	strcpy(opt_keys[num_opts], name);
	strcpy(opt_values[num_opts], value);
	num_opts++;
	return 1;
}
