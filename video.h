void create_window(int width, int height);
void video_configure(const struct retro_game_geometry *geom);
bool video_set_pixel_format(unsigned format);
void video_refresh(const void *data, unsigned width, unsigned height, size_t pitch);
void video_render();
void video_deinit();
