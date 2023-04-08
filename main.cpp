#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/glad.h>

#include <ggponet.h>
#if defined(_WIN32)
#  include "platform_windows.h"
#else
#  include "platform_unix.h"
#endif

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
#include "netplay.h"

extern GGPOSession *ggpo;
extern NonGameState ngs;
extern GLFWwindow *window;
config g_cfg;

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

int max(int a, int b)
{
	return a > b ? a : b;
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

	GGPOErrorCode result;
	
	// Fill in a ggpo callbacks structure to pass to start_session.
	GGPOSessionCallbacks cb = { 0 };
	cb.begin_game      = net_begin_game_callback;
	cb.advance_frame   = net_advance_frame_callback;
	cb.load_game_state = net_load_game_state_callback;
	cb.save_game_state = net_save_game_state_callback;
	cb.free_buffer     = net_free_buffer;
	cb.on_event        = net_on_event_callback;
	cb.log_game_state  = net_log_game_state;

	int pnum = argc > 1 ? atoi(argv[1]) : 1;
	int localport = 1233+pnum;
	int num_players = 4;
	int num_spectators = 0;
	ngs.num_players = num_players;

#if defined(_WIN32)
	WSADATA wd = { 0 };
	WSAStartup(MAKEWORD(2, 2), &wd);
#endif

	result = ggpo_start_session(&ggpo, &cb, "vectorwar", num_players, sizeof(uint16_t), localport);
	if (result != GGPO_OK)
		die("ggpo_start_session failed: %d\n", result);

	ggpo_set_disconnect_timeout(ggpo, 3000);
	ggpo_set_disconnect_notify_start(ggpo, 1000);

	GGPOPlayer players[4] = {{0}};
	switch (pnum) {
	case 1: {
		players[0].size = sizeof(GGPOPlayer);
		players[0].player_num = 1;
		players[0].type = GGPO_PLAYERTYPE_LOCAL;

		players[1].size = sizeof(GGPOPlayer);
		players[1].player_num = 2;
		players[1].type = GGPO_PLAYERTYPE_REMOTE;
		players[1].u.remote.port = 1235;
		strcpy(players[1].u.remote.ip_address, "127.0.0.1");

		players[2].size = sizeof(GGPOPlayer);
		players[2].player_num = 3;
		players[2].type = GGPO_PLAYERTYPE_REMOTE;
		players[2].u.remote.port = 1236;
		strcpy(players[2].u.remote.ip_address, "127.0.0.1");

		players[3].size = sizeof(GGPOPlayer);
		players[3].player_num = 4;
		players[3].type = GGPO_PLAYERTYPE_REMOTE;
		players[3].u.remote.port = 1237;
		strcpy(players[3].u.remote.ip_address, "127.0.0.1");
	}
	break;
	case 2: {
		players[0].size = sizeof(GGPOPlayer);
		players[0].player_num = 1;
		players[0].type = GGPO_PLAYERTYPE_REMOTE;
		players[0].u.remote.port = 1234;
		strcpy(players[0].u.remote.ip_address, "127.0.0.1");

		players[1].size = sizeof(GGPOPlayer);
		players[1].player_num = 2;
		players[1].type = GGPO_PLAYERTYPE_LOCAL;

		players[2].size = sizeof(GGPOPlayer);
		players[2].player_num = 3;
		players[2].type = GGPO_PLAYERTYPE_REMOTE;
		players[2].u.remote.port = 1236;
		strcpy(players[2].u.remote.ip_address, "127.0.0.1");
		
		players[3].size = sizeof(GGPOPlayer);
		players[3].player_num = 4;
		players[3].type = GGPO_PLAYERTYPE_REMOTE;
		players[3].u.remote.port = 1237;
		strcpy(players[3].u.remote.ip_address, "127.0.0.1");
	}
	break;
	case 3: {
		players[0].size = sizeof(GGPOPlayer);
		players[0].player_num = 1;
		players[0].type = GGPO_PLAYERTYPE_REMOTE;
		players[0].u.remote.port = 1234;
		strcpy(players[0].u.remote.ip_address, "127.0.0.1");

		players[1].size = sizeof(GGPOPlayer);
		players[1].player_num = 2;
		players[1].type = GGPO_PLAYERTYPE_REMOTE;
		players[1].u.remote.port = 1235;
		strcpy(players[1].u.remote.ip_address, "127.0.0.1");

		players[2].size = sizeof(GGPOPlayer);
		players[2].player_num = 3;
		players[2].type = GGPO_PLAYERTYPE_LOCAL;

		players[3].size = sizeof(GGPOPlayer);
		players[3].player_num = 4;
		players[3].type = GGPO_PLAYERTYPE_REMOTE;
		players[3].u.remote.port = 1237;
		strcpy(players[3].u.remote.ip_address, "127.0.0.1");
	}
	break;
	case 4: {
		players[0].size = sizeof(GGPOPlayer);
		players[0].player_num = 1;
		players[0].type = GGPO_PLAYERTYPE_REMOTE;
		players[0].u.remote.port = 1234;
		strcpy(players[0].u.remote.ip_address, "127.0.0.1");

		players[1].size = sizeof(GGPOPlayer);
		players[1].player_num = 2;
		players[1].type = GGPO_PLAYERTYPE_REMOTE;
		players[1].u.remote.port = 1235;
		strcpy(players[1].u.remote.ip_address, "127.0.0.1");

		players[2].size = sizeof(GGPOPlayer);
		players[2].player_num = 3;
		players[2].type = GGPO_PLAYERTYPE_REMOTE;
		players[2].u.remote.port = 1236;
		strcpy(players[2].u.remote.ip_address, "127.0.0.1");

		players[3].size = sizeof(GGPOPlayer);
		players[3].player_num = 4;
		players[3].type = GGPO_PLAYERTYPE_LOCAL;
	}
	break;
	}

	int i;
	for (i = 0; i < num_players + num_spectators; i++) {
		GGPOPlayerHandle handle;
		result = ggpo_add_player(ggpo, players + i, &handle);
		ngs.players[i].handle = handle;
		ngs.players[i].type = players[i].type;
		if (players[i].type == GGPO_PLAYERTYPE_LOCAL) {
			ngs.players[i].connect_progress = 100;
			ngs.local_player_handle = handle;
			ngs.SetConnectState(handle, Connecting);
			ggpo_set_frame_delay(ggpo, handle, 2);
		} else {
			ngs.players[i].connect_progress = 0;
		}
	}

	glfwSwapInterval(g_cfg.swap_interval);

	uint32_t start = Platform::GetCurrentTimeMS();
	uint32_t next = start;
	uint32_t now = start;

	while (!glfwWindowShouldClose(window)) {
		now = Platform::GetCurrentTimeMS();
		net_idle(max(0, next - now - 1));
		if (now >= next) {
			glfwPollEvents();
			net_run_frame();
			glfwSwapBuffers(window);
			// attempting to run the game at 120fps
			// works well enough on Windows for now
			// the game will still block on audio anyway
			next = now + (1000 / 120);
		}
	}

	// srm_save();
	core_unload();
	audio_deinit();
	video_deinit();

	glfwTerminate();
#ifdef _WIN32
	WSACleanup();
#endif
	return 0;
}
