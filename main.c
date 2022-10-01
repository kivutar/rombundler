#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/glad.h>

#include "mappings.h"
#include "config.h"
#include "options.h"
#include "core.h"
#include "audio.h"
#include "video.h"
#include "input.h"
#include "ini.h"
#include "srm.h"
#include "utils.h"

extern GLFWwindow *window;
config g_cfg;

static void error_cb(int error, const char* description)
{
	fprintf(stderr, "Error: %s\n", description);
}

int main(int argc, char *argv[]) {
	cfg_defaults(&g_cfg);
	if (ini_parse("./config.ini", cfg_handler, &g_cfg) < 0)
		die("Could not parse config.ini");

	ini_parse("./options.ini", opt_handler, NULL);

	glfwSetErrorCallback(error_cb);

	if (!glfwInit())
		die("Failed to initialize GLFW");

	if (!glfwUpdateGamepadMappings(mappings))
		die("Failed to load mappings");

	core_load(g_cfg.core);
	core_load_game(g_cfg.rom);

	srm_load();

	glfwSwapInterval(g_cfg.swap_interval);

	unsigned frame = 0;
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
		input_poll();
		core_run();
		video_render();
		glfwSwapBuffers(window);
		frame++;
		if (frame % 600 == 0)
			srm_save();
	}

	srm_save();
	core_unload();
	audio_deinit();
	video_deinit();

	glfwTerminate();
	return 0;
}
