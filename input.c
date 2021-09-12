#include "input.h"

struct keymap {
	unsigned k;
	unsigned rk;
};

struct keymap kbd_binds[] = {
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

struct keymap joy_binds[] = {
	{ GLFW_GAMEPAD_BUTTON_B, RETRO_DEVICE_ID_JOYPAD_A },
	{ GLFW_GAMEPAD_BUTTON_A, RETRO_DEVICE_ID_JOYPAD_B },
	{ GLFW_GAMEPAD_BUTTON_X, RETRO_DEVICE_ID_JOYPAD_Y },
	{ GLFW_GAMEPAD_BUTTON_Y, RETRO_DEVICE_ID_JOYPAD_X },
	{ GLFW_GAMEPAD_BUTTON_DPAD_UP, RETRO_DEVICE_ID_JOYPAD_UP },
	{ GLFW_GAMEPAD_BUTTON_DPAD_DOWN, RETRO_DEVICE_ID_JOYPAD_DOWN },
	{ GLFW_GAMEPAD_BUTTON_DPAD_LEFT, RETRO_DEVICE_ID_JOYPAD_LEFT },
	{ GLFW_GAMEPAD_BUTTON_DPAD_RIGHT, RETRO_DEVICE_ID_JOYPAD_RIGHT },
	{ GLFW_GAMEPAD_BUTTON_START, RETRO_DEVICE_ID_JOYPAD_START },
	{ GLFW_GAMEPAD_BUTTON_BACK, RETRO_DEVICE_ID_JOYPAD_SELECT },

	{ 0, 0 }
};

#define MAX_PLAYERS 5
static unsigned state[MAX_PLAYERS][RETRO_DEVICE_ID_JOYPAD_R3+1] = { 0 };
extern GLFWwindow *g_win;

void input_poll(void) {
	int i;
	for (i = 0; kbd_binds[i].k || kbd_binds[i].rk; ++i)
		state[0][kbd_binds[i].rk] = (glfwGetKey(g_win, kbd_binds[i].k) == GLFW_PRESS);

	int port;
	for (port = 0; port < MAX_PLAYERS; port++)
		if (glfwJoystickIsGamepad(port))
		{
			GLFWgamepadstate pad;
			if (glfwGetGamepadState(port, &pad))
				for (i = 0; i < 11; i++)
					state[port][joy_binds[i].rk] = pad.buttons[joy_binds[i].k];
		}

	if (glfwGetKey(g_win, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(g_win, true);
}

int16_t input_state(unsigned port, unsigned device, unsigned index, unsigned id) {
	if (port >= MAX_PLAYERS || index || device != RETRO_DEVICE_JOYPAD)
		return 0;

	return state[port][id];
}