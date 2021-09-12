#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>
#include <sys/time.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/glad.h>

#include "libretro.h"
#include "config.h"
#include "core.h"
#include "audio.h"
#include "video.h"
#include "input.h"
#include "ini.h"
#include "utils.h"

extern GLFWwindow *g_win;
extern struct retro_frame_time_callback runloop_frame_time;
retro_usec_t runloop_frame_time_last = 0;
config g_cfg;

static void error_cb(int error, const char* description)
{
	fprintf(stderr, "Error: %s\n", description);
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
		if (runloop_frame_time.callback) {
			retro_time_t current = get_time_usec();
			retro_time_t delta = current - runloop_frame_time_last;

			if (!runloop_frame_time_last)
				delta = runloop_frame_time.reference;
			runloop_frame_time_last = current;
			runloop_frame_time.callback(delta);
		}

		glfwPollEvents();

		core_run();

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
