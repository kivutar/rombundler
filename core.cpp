#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>
#include <sys/time.h>

#if defined(_WIN32)
#include <windows.h>
#else
#include <dlfcn.h>
#endif

#include "libretro.h"
#include "utils.h"
#include "video.h"
#include "audio.h"
#include "input.h"
#include "options.h"
#include "config.h"

#if defined(_WIN32)
#define load_lib(L) LoadLibrary(L);
#define load_sym(V, S) ((*(void**)&V) = GetProcAddress(core.handle, #S))
#define close_lib(L) //(L)
#else
#define load_sym(V, S) do {\
	if (!((*(void**)&V) = dlsym(core.handle, #S))) \
		die("Failed to load symbol '" #S "'': %s", dlerror()); \
	} while (0)
#define load_lib(L) dlopen(L, RTLD_LAZY);
#define close_lib(L) dlclose(L);
#endif
#define load_retro_sym(S) load_sym(core.S, S)

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
	void* (*retro_get_memory_data)(unsigned);
	size_t (*retro_get_memory_size)(unsigned);
	size_t (*retro_serialize_size)(void);
	bool (*retro_serialize)(void *data, size_t size);
	bool (*retro_unserialize)(const void *data, size_t size);
} core;

static struct retro_frame_time_callback runloop_frame_time;
static retro_usec_t runloop_frame_time_last = 0;
extern config g_cfg;

static void core_log(enum retro_log_level level, const char *fmt, ...)
{
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
}

static retro_time_t get_time_usec()
{
	struct timeval tv;
	gettimeofday(&tv,NULL);
	return tv.tv_sec*(int64_t)1000000+tv.tv_usec;
}

static bool core_environment(unsigned cmd, void *data)
{
	switch (cmd) {
		case RETRO_ENVIRONMENT_SET_ROTATION: {
			video_set_rotation(*(uintptr_t*)(data));
		}
		break;
		case RETRO_ENVIRONMENT_GET_LOG_INTERFACE: {
			struct retro_log_callback *cb = (struct retro_log_callback *)data;
			cb->log = core_log;
		}
		break;
		case RETRO_ENVIRONMENT_GET_CAN_DUPE: {
			*(bool*)data = true;
		}
		break;
		case RETRO_ENVIRONMENT_GET_LANGUAGE: {
			*(unsigned*)data = RETRO_LANGUAGE_ENGLISH;
		}
		break;
		case RETRO_ENVIRONMENT_GET_USERNAME: {
			*(const char**)data = NULL;
		}
		break;
		case RETRO_ENVIRONMENT_GET_FASTFORWARDING: {
			*(bool*)data = false;
		}
		break;
		case RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE: {
			*(bool*)data = false;
		}
		break;
		case RETRO_ENVIRONMENT_SHUTDOWN: {
			video_should_close(1);
		}
		break;
		case RETRO_ENVIRONMENT_GET_AUDIO_VIDEO_ENABLE: {
			*(int*)data = 1 << 0 | 1 << 1;
		}
		break;
		case RETRO_ENVIRONMENT_SET_FRAME_TIME_CALLBACK: {
			const struct retro_frame_time_callback *cb =
				(const struct retro_frame_time_callback*)data;
			runloop_frame_time = *cb;
		}
		break;
		case RETRO_ENVIRONMENT_SET_KEYBOARD_CALLBACK: {
			const struct retro_keyboard_callback *cb =
				(const struct retro_keyboard_callback*)data;
			input_set_keyboard_callback(cb->callback);
		}
		break;
		case RETRO_ENVIRONMENT_GET_PERF_INTERFACE: {
			struct retro_perf_callback *perf_cb = (struct retro_perf_callback*)data;
			perf_cb->get_time_usec = get_time_usec;
		}
		break;
		case RETRO_ENVIRONMENT_SET_GEOMETRY: {
			const struct retro_game_geometry *geom = (const struct retro_game_geometry*)data;
			video_set_geometry(geom);
		}
		break;
		case RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION: {
			*(unsigned *)data = 0;
		}
		break;
		case RETRO_ENVIRONMENT_GET_PREFERRED_HW_RENDER: {
			// printf("RETRO_ENVIRONMENT_GET_PREFERRED_HW_RENDER: RETRO_HW_CONTEXT_OPENGL\n");
			*(unsigned *)data = RETRO_HW_CONTEXT_OPENGL;
		}
		break;
		case RETRO_ENVIRONMENT_SET_HW_RENDER: {
			struct retro_hw_render_callback *hw = (struct retro_hw_render_callback*)data;
			hw->get_current_framebuffer = video_get_current_framebuffer;
			hw->get_proc_address = (retro_hw_get_proc_address_t)glfwGetProcAddress;
			video_set_hw(*hw);
			// printf("RETRO_ENVIRONMENT_SET_HW_RENDER: %d\n", hw->context_type);
		}
		break;
		case RETRO_ENVIRONMENT_SET_VARIABLES: {
			return true;
		}
		case RETRO_ENVIRONMENT_GET_VARIABLE: {
			struct retro_variable *var = (struct retro_variable*) data;
			return get_option(var->key, &var->value);
		}
		break;
		case RETRO_ENVIRONMENT_SET_PIXEL_FORMAT: {
			const enum retro_pixel_format *fmt = (enum retro_pixel_format *)data;
			return video_set_pixel_format(*fmt);
		}
		break;
		case RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY:
		case RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY:
		case RETRO_ENVIRONMENT_GET_CORE_ASSETS_DIRECTORY:
			*(const char **)data = ".";
		break;

		default:
			core_log(RETRO_LOG_DEBUG, "Unhandled env #%u", cmd);
			return false;
	}

	return true;
}

void input_poll_dummy(void) {}

void core_load(const char *sofile)
{
	void (*set_environment)(retro_environment_t) = NULL;
	void (*set_video_refresh)(retro_video_refresh_t) = NULL;
	void (*set_input_poll)(retro_input_poll_t) = NULL;
	void (*set_input_state)(retro_input_state_t) = NULL;
	void (*set_audio_sample)(retro_audio_sample_t) = NULL;
	void (*set_audio_sample_batch)(retro_audio_sample_batch_t) = NULL;

	memset(&core, 0, sizeof(core));
	core.handle = load_lib(sofile);

	if (!core.handle)
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
	load_retro_sym(retro_get_memory_data);
	load_retro_sym(retro_get_memory_size);
	load_retro_sym(retro_serialize_size);
	load_retro_sym(retro_serialize);
	load_retro_sym(retro_unserialize);

	load_sym(set_environment, retro_set_environment);
	load_sym(set_video_refresh, retro_set_video_refresh);
	load_sym(set_input_poll, retro_set_input_poll);
	load_sym(set_input_state, retro_set_input_state);
	load_sym(set_audio_sample, retro_set_audio_sample);
	load_sym(set_audio_sample_batch, retro_set_audio_sample_batch);

	set_environment(core_environment);
	core.retro_init();
	set_video_refresh(video_refresh);
	set_input_poll(input_poll_dummy);
	set_input_state(input_state);
	set_audio_sample(audio_sample);
	set_audio_sample_batch(audio_sample_batch);

	core.initialized = true;
}

void core_load_game(const char *filename)
{
	struct retro_system_av_info av = {{0}};
	struct retro_system_info si = {0};
	struct retro_game_info info = { filename, 0 };
	FILE *file = fopen(filename, "rb");

	if (!file)
		die("The core could not open the file.");

	fseek(file, 0, SEEK_END);
	info.size = ftell(file);
	rewind(file);

	core.retro_get_system_info(&si);

	if (!si.need_fullpath) {
		info.data = malloc(info.size);

		if (!info.data || !fread((void*)info.data, info.size, 1, file))
			die("The core could not read the file.");
	}

	if (!core.retro_load_game(&info))
		die("The core failed to load the content.");

	core.retro_get_system_av_info(&av);

	video_configure(&av.geometry);
	audio_init(av.timing.sample_rate);

	if (g_cfg.port0) core.retro_set_controller_port_device(0, g_cfg.port0);
	if (g_cfg.port1) core.retro_set_controller_port_device(1, g_cfg.port1);
	if (g_cfg.port2) core.retro_set_controller_port_device(2, g_cfg.port2);
	if (g_cfg.port3) core.retro_set_controller_port_device(3, g_cfg.port3);

	return;
}

void core_run()
{
	if (runloop_frame_time.callback) {
		retro_time_t current = get_time_usec();
		retro_time_t delta = current - runloop_frame_time_last;

		if (!runloop_frame_time_last)
			delta = runloop_frame_time.reference;
		runloop_frame_time_last = current;
		runloop_frame_time.callback(delta);
	}

	core.retro_run();
}

size_t core_get_memory_size(unsigned id)
{
	return core.retro_get_memory_size(id);
}

void *core_get_memory_data(unsigned id)
{
	return core.retro_get_memory_data(id);
}

size_t core_serialize_size()
{
	printf("core_serialize_size\n");
	return core.retro_serialize_size();
}

bool core_serialize(void *data, size_t size)
{
	printf("core_serialize\n");
	return core.retro_serialize(data, size);
}

bool core_unserialize(const void *data, size_t size)
{
	printf("core_unserialize\n");
	return core.retro_unserialize(data, size);
}

void core_unload()
{
	if (core.initialized)
		core.retro_deinit();

	if (core.handle)
		close_lib(core.handle);
}
