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
} core;

static struct retro_frame_time_callback runloop_frame_time;
static retro_usec_t runloop_frame_time_last = 0;

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

static retro_time_t get_time_usec() {
	struct timeval tv;
	gettimeofday(&tv,NULL);
	return tv.tv_sec*(int64_t)1000000+tv.tv_usec;
}



static bool core_environment(unsigned cmd, void *data) {
	switch (cmd) {
		case RETRO_ENVIRONMENT_GET_LOG_INTERFACE: {
			struct retro_log_callback *cb = (struct retro_log_callback *)data;
			cb->log = core_log;
		}
		break;
		case RETRO_ENVIRONMENT_GET_CAN_DUPE: {
			bool *bval = (bool*)data;
			*bval = true;
		}
		break;
		case RETRO_ENVIRONMENT_GET_FASTFORWARDING: {
			bool *bval = (bool*)data;
			*bval = false;
		}
		break;
		case RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE: {
			bool *bval = (bool*)data;
			*bval = false;
		}
		break;
		case RETRO_ENVIRONMENT_GET_AUDIO_VIDEO_ENABLE: {
			int *value = (int*)data;
			*value = 1 << 0 | 1 << 1;
		}
		break;
		case RETRO_ENVIRONMENT_SET_FRAME_TIME_CALLBACK: {
			const struct retro_frame_time_callback *frame_time =
				(const struct retro_frame_time_callback*)data;
			runloop_frame_time = *frame_time;
		}
		break;
		case RETRO_ENVIRONMENT_GET_PERF_INTERFACE: {
			struct retro_perf_callback *perf_cb = (struct retro_perf_callback*)data;
			perf_cb->get_time_usec = get_time_usec;
		}
		break;
		case RETRO_ENVIRONMENT_GET_VARIABLE: {
			struct retro_variable *var = (struct retro_variable*) data;
			return get_option(var->key, &var->value);
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

void input_poll_dummy(void) {}

void core_load(const char *sofile) {
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

void core_load_game(const char *filename) {
	struct retro_system_av_info av = {0};
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

	return;
}

void core_run() {
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

void core_unload() {
	if (core.initialized)
		core.retro_deinit();

	if (core.handle)
		close_lib(core.handle);
}


