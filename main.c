#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#define GL_GLEXT_PROTOTYPES
#define EGL_EGLEXT_PROTOTYPES
#else
#include <glad/glad.h>
#endif

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
static unsigned frame = 0;

static void error_cb(int error, const char* description)
{
	fprintf(stderr, "Error: %s\n", description);
}

static void update()
{
	glfwPollEvents();
	input_poll();
	core_run();
	video_render();
	glfwSwapBuffers(window);
	frame++;
	if (frame % 600 == 0)
		srm_save();
}

int main(int argc, char *argv[]) {
	cfg_defaults(&g_cfg);
	if (ini_parse("./config.ini", cfg_handler, &g_cfg) < 0)
		die("Could not parse config.ini");

	ini_parse("./options.ini", opt_handler, NULL);

	glfwSetErrorCallback(error_cb);

	if (!glfwInit())
		die("Failed to initialize GLFW");

	core_load(g_cfg.core);
	core_load_game(g_cfg.rom);

	srm_load();

	glfwSwapInterval(g_cfg.swap_interval);

#ifdef __EMSCRIPTEN__
	emscripten_set_main_loop(update, 0, true);
#else
	while (!glfwWindowShouldClose(window))
		update();
#endif

	srm_save();
	core_unload();
	audio_deinit();
	video_deinit();

	glfwTerminate();
	return 0;
}
