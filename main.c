#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>

#if defined(_WIN32)
#include <windows.h>
#else
#include <dlfcn.h>
#endif

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/glad.h>

#include "libretro.h"
#include "config.h"
#include "audio.h"
#include "video.h"
#include "ini.h"
#include "utils.h"

extern GLFWwindow *g_win;
config g_cfg;

static struct {
	void *handle;
	bool initialized;

	void (*retro_init)(void);
	void (*retro_deinit)(void);
	unsigned (*retro_api_version)(void);
	void (*retro_get_system_info)(struct retro_system_info *info);
	void (*retro_get_system_av_info)(struct retro_system_av_info *info);
	void (*retro_set_controller_port_device)(unsigned port, unsigned device);
	void (*retro_reset)(void);
	void (*retro_run)(void);
	bool (*retro_load_game)(const struct retro_game_info *game);
	void (*retro_unload_game)(void);
} g_retro;

struct keymap {
	unsigned k;
	unsigned rk;
};

struct keymap g_binds[] = {
	{ GLFW_KEY_X, RETRO_DEVICE_ID_JOYPAD_A },
	{ GLFW_KEY_Z, RETRO_DEVICE_ID_JOYPAD_B },
	{ GLFW_KEY_A, RETRO_DEVICE_ID_JOYPAD_Y },
	{ GLFW_KEY_S, RETRO_DEVICE_ID_JOYPAD_X },
	{ GLFW_KEY_UP, RETRO_DEVICE_ID_JOYPAD_UP },
	{ GLFW_KEY_DOWN, RETRO_DEVICE_ID_JOYPAD_DOWN },
	{ GLFW_KEY_LEFT, RETRO_DEVICE_ID_JOYPAD_LEFT },
	{ GLFW_KEY_RIGHT, RETRO_DEVICE_ID_JOYPAD_RIGHT },
	{ GLFW_KEY_ENTER, RETRO_DEVICE_ID_JOYPAD_START },
	{ GLFW_KEY_BACKSPACE, RETRO_DEVICE_ID_JOYPAD_SELECT },

	{ 0, 0 }
};

static unsigned g_joy[RETRO_DEVICE_ID_JOYPAD_R3+1] = { 0 };

#if defined(_WIN32)
#define load_lib(L) LoadLibrary(L);
#define load_sym(V, S) ((*(void**)&V) = GetProcAddress(g_retro.handle, #S))
#define close_lib(L) //(L)
#else
#define load_sym(V, S) do {\
	if (!((*(void**)&V) = dlsym(g_retro.handle, #S))) \
		die("Failed to load symbol '" #S "'': %s", dlerror()); \
	} while (0)
#define load_lib(L) dlopen(L, RTLD_LAZY);
#define close_lib(L) dlclose(L);
#endif
#define load_retro_sym(S) load_sym(g_retro.S, S)

static void error_cb(int error, const char* description)
{
	fprintf(stderr, "Error: %s\n", description);
}

static void core_log(enum retro_log_level level, const char *fmt, ...) {
	char buffer[4096] = {0};
	static const char * levelstr[] = { "dbg", "inf", "wrn", "err" };
	va_list va;

	va_start(va, fmt);
	vsnprintf(buffer, sizeof(buffer), fmt, va);
	va_end(va);

	if (level == 0)
		return;

	fprintf(stderr, "[%s] %s", levelstr[level], buffer);
	fflush(stderr);

	if (level == RETRO_LOG_ERROR)
		exit(EXIT_FAILURE);
}

static bool core_environment(unsigned cmd, void *data) {
	switch (cmd) {
		case RETRO_ENVIRONMENT_GET_LOG_INTERFACE: {
			struct retro_log_callback *cb = (struct retro_log_callback *)data;
			cb->log = core_log;
			break;
		}
		case RETRO_ENVIRONMENT_GET_CAN_DUPE: {
			*(bool*)data = true;
			break;
		}
		case RETRO_ENVIRONMENT_SET_PIXEL_FORMAT: {
			const enum retro_pixel_format *fmt = (enum retro_pixel_format *)data;

			if (*fmt > RETRO_PIXEL_FORMAT_RGB565)
				return false;

			return video_set_pixel_format(*fmt);
		}
		case RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY:
		case RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY:
			*(const char **)data = ".";
			return true;

		default:
			core_log(RETRO_LOG_DEBUG, "Unhandled env #%u", cmd);
			return false;
	}

	return true;
}

static void core_video_refresh(const void *data, unsigned width, unsigned height, size_t pitch) {
	if (data)
		video_refresh(data, width, height, pitch);
}

static void core_input_poll(void) {
	int i;
	for (i = 0; g_binds[i].k || g_binds[i].rk; ++i)
		g_joy[g_binds[i].rk] = (glfwGetKey(g_win, g_binds[i].k) == GLFW_PRESS);

	if (glfwGetKey(g_win, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(g_win, true);
}

static int16_t core_input_state(unsigned port, unsigned device, unsigned index, unsigned id) {
	if (port || index || device != RETRO_DEVICE_JOYPAD)
		return 0;

	return g_joy[id];
}

static void core_audio_sample(int16_t left, int16_t right) {
	int16_t buf[2] = {left, right};
	audio_write(buf, 4);
}

static size_t core_audio_sample_batch(const int16_t *data, size_t frames) {
	return audio_write(data, frames*4);
}

static void core_load(const char *sofile) {
	void (*set_environment)(retro_environment_t) = NULL;
	void (*set_video_refresh)(retro_video_refresh_t) = NULL;
	void (*set_input_poll)(retro_input_poll_t) = NULL;
	void (*set_input_state)(retro_input_state_t) = NULL;
	void (*set_audio_sample)(retro_audio_sample_t) = NULL;
	void (*set_audio_sample_batch)(retro_audio_sample_batch_t) = NULL;

	memset(&g_retro, 0, sizeof(g_retro));
	g_retro.handle = load_lib(sofile);

	if (!g_retro.handle)
		die("Failed to load core");

	load_retro_sym(retro_init);
	load_retro_sym(retro_deinit);
	load_retro_sym(retro_api_version);
	load_retro_sym(retro_get_system_info);
	load_retro_sym(retro_get_system_av_info);
	load_retro_sym(retro_set_controller_port_device);
	load_retro_sym(retro_reset);
	load_retro_sym(retro_run);
	load_retro_sym(retro_load_game);
	load_retro_sym(retro_unload_game);

	load_sym(set_environment, retro_set_environment);
	load_sym(set_video_refresh, retro_set_video_refresh);
	load_sym(set_input_poll, retro_set_input_poll);
	load_sym(set_input_state, retro_set_input_state);
	load_sym(set_audio_sample, retro_set_audio_sample);
	load_sym(set_audio_sample_batch, retro_set_audio_sample_batch);

	set_environment(core_environment);
	set_video_refresh(core_video_refresh);
	set_input_poll(core_input_poll);
	set_input_state(core_input_state);
	set_audio_sample(core_audio_sample);
	set_audio_sample_batch(core_audio_sample_batch);

	g_retro.retro_init();
	g_retro.initialized = true;
}

static void core_load_game(const char *filename) {
	struct retro_system_av_info av = {0};
	struct retro_system_info si = {0};
	struct retro_game_info info = { filename, 0 };
	FILE *file = fopen(filename, "rb");

	if (!file)
		goto libc_error;

	fseek(file, 0, SEEK_END);
	info.size = ftell(file);
	rewind(file);

	g_retro.retro_get_system_info(&si);

	if (!si.need_fullpath) {
		info.data = malloc(info.size);

		if (!info.data || !fread((void*)info.data, info.size, 1, file))
			goto libc_error;
	}

	if (!g_retro.retro_load_game(&info))
		die("The core failed to load the content.");

	g_retro.retro_get_system_av_info(&av);

	video_configure(&av.geometry);
	audio_init(av.timing.sample_rate);

	return;

libc_error:
	die("Failed to load content '%s': %s", filename, strerror(errno));
}

static void core_unload() {
	if (g_retro.initialized)
		g_retro.retro_deinit();

	if (g_retro.handle)
		close_lib(g_retro.handle);
}

int main(int argc, char *argv[]) {
	if (ini_parse("config.ini", handler, &g_cfg) < 0)
		die("Could not parse ini");

	glfwSetErrorCallback(error_cb);

	if (!glfwInit())
		die("Failed to initialize GLFW");

	core_load(g_cfg.core);
	core_load_game(g_cfg.rom);

	while (!glfwWindowShouldClose(g_win)) {
		glfwPollEvents();

		g_retro.retro_run();

		glClear(GL_COLOR_BUFFER_BIT);

		video_render();

		glfwSwapBuffers(g_win);
	}

	core_unload();
	audio_deinit();
	video_deinit();

	glfwTerminate();
	return 0;
}
