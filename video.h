#include "libretro.h"

void create_window(int width, int height);
void video_configure(const struct retro_game_geometry *geom);
bool video_set_pixel_format(unsigned format);
void video_set_geometry(const struct retro_game_geometry *geom);
void video_set_hw(struct retro_hw_render_callback);
void video_should_close(int v);
void video_refresh(const void *data, unsigned width, unsigned height, size_t pitch);
uintptr_t video_get_current_framebuffer();
void video_render();
void video_deinit();
