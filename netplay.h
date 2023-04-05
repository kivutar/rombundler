bool __cdecl net_begin_game_callback(const char *name);
void net_idle(int time);
void net_run_frame();
bool __cdecl net_advance_frame_callback(int flags);
bool __cdecl net_load_game_state_callback(unsigned char *buffer, int len);
bool __cdecl net_save_game_state_callback(unsigned char **buffer, int *len, int *checksum, int frame);
void __cdecl net_free_buffer(void *buffer);
bool __cdecl net_on_event_callback(GGPOEvent *info);
bool __cdecl net_log_game_state(char *filename, unsigned char *buffer, int len);
