#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <ggponet.h>

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
#include "nongamestate.h"

#include <platform_unix.h>

GGPOSession *ggpo = NULL;
NonGameState ngs = { 0 };
extern GLFWwindow *window;
config g_cfg;

static bool
vw_on_event_callback(GGPOEvent *info)
{
   int progress;
   switch (info->code) {
   case GGPO_EVENTCODE_CONNECTED_TO_PEER:
      ngs.SetConnectState(info->u.connected.player, Synchronizing);
      break;
   case GGPO_EVENTCODE_SYNCHRONIZING_WITH_PEER:
      progress = 100 * info->u.synchronizing.count / info->u.synchronizing.total;
      ngs.UpdateConnectProgress(info->u.synchronizing.player, progress);
      break;
   case GGPO_EVENTCODE_SYNCHRONIZED_WITH_PEER:
      ngs.UpdateConnectProgress(info->u.synchronized.player, 100);
      break;
   case GGPO_EVENTCODE_RUNNING:
      ngs.SetConnectState(Running);
      // renderer->SetStatusText("");
      break;
   case GGPO_EVENTCODE_CONNECTION_INTERRUPTED:
      ngs.SetDisconnectTimeout(info->u.connection_interrupted.player,
                               Platform::GetCurrentTimeMS(),
                               info->u.connection_interrupted.disconnect_timeout);
      break;
   case GGPO_EVENTCODE_CONNECTION_RESUMED:
      ngs.SetConnectState(info->u.connection_resumed.player, Running);
      break;
   case GGPO_EVENTCODE_DISCONNECTED_FROM_PEER:
      ngs.SetConnectState(info->u.disconnected.player, Disconnected);
      break;
   case GGPO_EVENTCODE_TIMESYNC:
	  usleep(1000 * info->u.timesync.frames_ahead / 60);
      break;
   }
   return true;
}

static void error_cb(int error, const char* description)
{
	fprintf(stderr, "Error: %s\n", description);
}

void joystick_callback(int jid, int event)
{
	if (event == GLFW_CONNECTED)
		printf("%s %s\n", glfwGetGamepadName(jid), glfwGetJoystickGUID(jid));
	else if (event == GLFW_DISCONNECTED)
		printf("Joypad %d disconnected\n", jid);
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
	else
		printf("Updated mappings\n");

	glfwSetJoystickCallback(joystick_callback);

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
