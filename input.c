#include "input.h"
#include "config.h"

extern config g_cfg;

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
	{ GLFW_KEY_RIGHT_SHIFT, RETRO_DEVICE_ID_JOYPAD_SELECT },

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
	{ GLFW_GAMEPAD_BUTTON_LEFT_BUMPER, RETRO_DEVICE_ID_JOYPAD_L },
	{ GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER, RETRO_DEVICE_ID_JOYPAD_R },
	{ GLFW_GAMEPAD_BUTTON_LEFT_THUMB, RETRO_DEVICE_ID_JOYPAD_L3 },
	{ GLFW_GAMEPAD_BUTTON_RIGHT_THUMB, RETRO_DEVICE_ID_JOYPAD_R3 },

	{ 0, 0 }
};

#define MAX_PLAYERS 5
static unsigned state[MAX_PLAYERS][RETRO_DEVICE_ID_JOYPAD_R3+1] = { 0 };
extern GLFWwindow *window;

void input_poll(void) {
	int i;
	for (i = 0; kbd_binds[i].k || kbd_binds[i].rk; ++i)
		state[0][kbd_binds[i].rk] = (glfwGetKey(window, kbd_binds[i].k) == GLFW_PRESS);

	int port;
	for (port = 0; port < MAX_PLAYERS; port++)
		if (glfwJoystickIsGamepad(port))
		{
			GLFWgamepadstate pad;
			if (glfwGetGamepadState(port, &pad))
				for (i = 0; i < 11; i++)
					state[port][joy_binds[i].rk] = pad.buttons[joy_binds[i].k];

			int count;
			const float *axes = glfwGetJoystickAxes(port, &count);
			if (g_cfg.map_analog_to_dpad)
			{
				if (count >= 2)
				{
					state[port][RETRO_DEVICE_ID_JOYPAD_LEFT] = axes[GLFW_GAMEPAD_AXIS_LEFT_X] < -0.5;
					state[port][RETRO_DEVICE_ID_JOYPAD_RIGHT] = axes[GLFW_GAMEPAD_AXIS_LEFT_X] > 0.5;
					state[port][RETRO_DEVICE_ID_JOYPAD_UP] = axes[GLFW_GAMEPAD_AXIS_LEFT_Y] < -0.5;
					state[port][RETRO_DEVICE_ID_JOYPAD_DOWN] = axes[GLFW_GAMEPAD_AXIS_LEFT_Y] > 0.5;
				}
			}

			state[port][RETRO_DEVICE_ID_JOYPAD_L2] = axes[GLFW_GAMEPAD_AXIS_LEFT_TRIGGER] > 0.5;
			state[port][RETRO_DEVICE_ID_JOYPAD_R2] = axes[GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER] > 0.5;
		}

	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);
}

static double oldx = 0;
static double oldy = 0;

int16_t input_state(unsigned port, unsigned device, unsigned index, unsigned id) {
	if (port < MAX_PLAYERS && device == RETRO_DEVICE_JOYPAD)
		return state[port][id];

	if (device == RETRO_DEVICE_MOUSE) {
		double x = 0;
		double y = 0;
		glfwGetCursorPos(window, &x, &y);
		if (id == RETRO_DEVICE_ID_MOUSE_X)
		{
			int16_t d = x - oldx;
			oldx = x;
			return d;
		}
		if (id == RETRO_DEVICE_ID_MOUSE_Y)
		{
			int16_t d = y - oldy;
			oldy = y;
			return d;
		}
		if (id == RETRO_DEVICE_ID_MOUSE_LEFT && glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_1) == GLFW_PRESS)
			return 1;
	}

	return 0;
}
