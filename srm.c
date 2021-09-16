#include <stdio.h>
#include "libretro.h"
#include "core.h"

void srm_save()
{
	size_t size = core_get_memory_size(RETRO_MEMORY_SAVE_RAM);
	void *data = core_get_memory_data(RETRO_MEMORY_SAVE_RAM);
	if (!size || !data)
		return;

	FILE *f = fopen("./save.srm", "w");
	if (!f)
		return;
	
	fwrite(data, 1, size, f);
	fclose(f);
}

void srm_load()
{
	size_t size = core_get_memory_size(RETRO_MEMORY_SAVE_RAM);
	void *data = core_get_memory_data(RETRO_MEMORY_SAVE_RAM);
	if (!size || !data)
		return;

	FILE *f = fopen("./save.srm", "r+");
	if (!f)
		return;
	
	fread(data, 1, size, f);
	fclose(f);
}
