#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>
#include <sys/time.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/glad.h>

#include "config.h"
#include "options.h"
#include "core.h"
#include "audio.h"
#include "video.h"
#include "input.h"
#include "ini.h"
#include "utils.h"

extern GLFWwindow *window;
config g_cfg;

static void error_cb(int error, const char* description)
{
	fprintf(stderr, "Error: %s\n", description);
}

int main(int argc, char *argv[]) {
	if (ini_parse("config.ini", cfg_handler, &g_cfg) < 0)
		die("Could not parse config.ini");

	ini_parse("options.ini", opt_handler, NULL);

	glfwSetErrorCallback(error_cb);

	if (!glfwInit())
		die("Failed to initialize GLFW");

	core_load(g_cfg.core);
	core_load_game(g_cfg.rom);

	glfwSwapInterval(g_cfg.swap_interval);

	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();

		input_poll();

		core_run();

		glClear(GL_COLOR_BUFFER_BIT);

		video_render();

		glfwSwapBuffers(window);
	}

	core_unload();
	audio_deinit();
	video_deinit();

	glfwTerminate();
	return 0;
}
