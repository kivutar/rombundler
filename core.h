void core_load(const char *sofile);
void core_load_game(const char *filename);
size_t core_get_memory_size(unsigned id);
void *core_get_memory_data(unsigned id);
void core_run();
void core_unload();
