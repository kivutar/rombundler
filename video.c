#include <assert.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/glad.h>

#include "libretro.h"
#include "config.h"
#include "utils.h"

GLFWwindow *window = NULL;
extern config g_cfg;

static GLfloat g_vertex[] = {
	-1.0f, -1.0f, // left-bottom
	-1.0f,  1.0f, // left-top
	 1.0f, -1.0f, // right-bottom
	 1.0f,  1.0f, // right-top
};

static GLfloat g_texcoords[] ={
	0.0f,  1.0f,
	0.0f,  0.0f,
	1.0f,  1.0f,
	1.0f,  0.0f,
};

static struct {
	GLuint tex_id;
	GLuint pitch;
	GLint tex_w, tex_h;
	GLuint clip_w, clip_h;

	GLuint pixfmt;
	GLuint pixtype;
	GLuint bpp;
} video = {0};

static void resize_cb(GLFWwindow *win, int w, int h) {
	glViewport(0, 0, w, h);
}

void create_window(int width, int height) {
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

	GLFWmonitor* monitor = NULL;
	if (g_cfg.fullscreen)
		monitor = glfwGetPrimaryMonitor();

	window = glfwCreateWindow(width, height, g_cfg.title, monitor, NULL);

	if (!window)
		die("Failed to create window.");

	glfwSetFramebufferSizeCallback(window, resize_cb);

	glfwMakeContextCurrent(window);

	if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress))
		die("Failed to initialize glad");

	glfwSwapInterval(1);

	glEnable(GL_TEXTURE_2D);

	resize_cb(window, width, height);
}

static void refresh_vertex_data() {
	assert(video.tex_w);
	assert(video.tex_h);
	assert(video.clip_w);
	assert(video.clip_h);

	GLfloat *coords = g_texcoords;
	coords[1] = coords[5] = (float)video.clip_h / video.tex_h;
	coords[4] = coords[6] = (float)video.clip_w / video.tex_w;
}

static void resize_to_aspect(double ratio, int sw, int sh, int *dw, int *dh) {
	*dw = sw;
	*dh = sh;

	if (ratio <= 0)
		ratio = (double)sw / sh;

	if ((float)sw / sh < 1)
		*dw = *dh * ratio;
	else
		*dh = *dw / ratio;
}

void video_configure(const struct retro_game_geometry *geom) {
	int nwidth, nheight;

	resize_to_aspect(geom->aspect_ratio, geom->base_width * 1, geom->base_height * 1, &nwidth, &nheight);

	nwidth *= g_cfg.scale;
	nheight *= g_cfg.scale;

	if (!window)
		create_window(nwidth, nheight);

	if (video.tex_id)
		glDeleteTextures(1, &video.tex_id);

	video.tex_id = 0;

	if (!video.pixfmt)
		video.pixfmt = GL_UNSIGNED_SHORT_5_5_5_1;

	glfwSetWindowSize(window, nwidth, nheight);

	glGenTextures(1, &video.tex_id);

	if (!video.tex_id)
		die("Failed to create the video texture");

	video.pitch = geom->base_width * video.bpp;

	glBindTexture(GL_TEXTURE_2D, video.tex_id);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, geom->max_width, geom->max_height, 0,
			video.pixtype, video.pixfmt, NULL);

	glBindTexture(GL_TEXTURE_2D, 0);

	video.tex_w = geom->max_width;
	video.tex_h = geom->max_height;
	video.clip_w = geom->base_width;
	video.clip_h = geom->base_height;

	refresh_vertex_data();
}

bool video_set_pixel_format(unsigned format) {
	if (video.tex_id)
		die("Tried to change pixel format after initialization.");

	switch (format) {
		case RETRO_PIXEL_FORMAT_0RGB1555:
			video.pixfmt = GL_UNSIGNED_SHORT_5_5_5_1;
			video.pixtype = GL_BGRA;
			video.bpp = sizeof(uint16_t);
			break;
		case RETRO_PIXEL_FORMAT_XRGB8888:
			video.pixfmt = GL_UNSIGNED_INT_8_8_8_8_REV;
			video.pixtype = GL_BGRA;
			video.bpp = sizeof(uint32_t);
			break;
		case RETRO_PIXEL_FORMAT_RGB565:
			video.pixfmt  = GL_UNSIGNED_SHORT_5_6_5;
			video.pixtype = GL_RGB;
			video.bpp = sizeof(uint16_t);
			break;
		default:
			die("Unknown pixel type %u", format);
	}

	return true;
}

void video_deinit() {
	if (video.tex_id)
		glDeleteTextures(1, &video.tex_id);

	video.tex_id = 0;
}

void video_render() {
	glBindTexture(GL_TEXTURE_2D, video.tex_id);

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	glVertexPointer(2, GL_FLOAT, 0, g_vertex);
	glTexCoordPointer(2, GL_FLOAT, 0, g_texcoords);

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void video_refresh(const void *data, unsigned width, unsigned height, size_t pitch) {
	if (!data || !pitch)
		return;

	if (video.clip_w != width || video.clip_h != height) {
		video.clip_h = height;
		video.clip_w = width;

		refresh_vertex_data();
	}

	glBindTexture(GL_TEXTURE_2D, video.tex_id);

	if (pitch != video.pitch) {
		video.pitch = pitch;
		glPixelStorei(GL_UNPACK_ROW_LENGTH, video.pitch / video.bpp);
	}

	if (data)
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, video.pixtype, video.pixfmt, data);
}
