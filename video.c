#include <stdio.h>
#include <assert.h>
#include <string.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/glad.h>

#include "libretro.h"
#include "config.h"
#include "utils.h"
#include "shaders.h"

#ifdef __APPLE__
#define glGenVertexArrays glGenVertexArraysAPPLE
#define glBindVertexArray glBindVertexArrayAPPLE
#define glDeleteVertexArrays glDeleteVertexArraysAPPLE
#define glBindFramebuffer glBindFramebufferEXT
#define glGenFramebuffers glGenFramebuffersEXT
#define glGenRenderbuffers glGenRenderbuffersEXT
#define glBindRenderbuffer glBindRenderbufferEXT
#define glFramebufferTexture2D glFramebufferTexture2DEXT
#define glCheckFramebufferStatus glCheckFramebufferStatusEXT
#define glRenderbufferStorage glRenderbufferStorageEXT
#define glFramebufferRenderbuffer glFramebufferRenderbufferEXT
#define GL_FRAMEBUFFER GL_FRAMEBUFFER_EXT
#define GL_COLOR_ATTACHMENT0 GL_COLOR_ATTACHMENT0_EXT
#define GL_RENDERBUFFER GL_RENDERBUFFER_EXT
#define GL_FRAMEBUFFER_COMPLETE GL_FRAMEBUFFER_COMPLETE_EXT
#define GL_DEPTH_ATTACHMENT GL_DEPTH_ATTACHMENT_EXT
#define GL_STENCIL_ATTACHMENT GL_STENCIL_ATTACHMENT_EXT
#define GL_DEPTH24_STENCIL8 GL_DEPTH24_STENCIL8_EXT
#endif

GLFWwindow *window = NULL;
extern config g_cfg;

static struct {
	GLuint tex_id;
	GLuint fbo_id;
	GLuint rbo_id;

	GLuint pitch;
	GLint tex_w, tex_h;
	GLuint clip_w, clip_h;

	GLuint pixfmt;
	GLuint pixtype;
	GLuint bpp;

	struct retro_hw_render_callback hw;
} video = {0};

static struct {
	GLuint vao;
	GLuint vbo;
	GLuint program;

	GLint i_pos;
	GLint i_coord;
	GLint u_tex;
	GLint u_tex_size;
	GLint u_mvp;
} shader = {0};

static GLuint compile_shader(unsigned type, unsigned count, const char **strings)
{
	GLuint shader = glCreateShader(type);
	glShaderSource(shader, count, strings, NULL);
	glCompileShader(shader);

	GLint status;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &status);

	if (status == GL_FALSE) {
		char buffer[4096];
		glGetShaderInfoLog(shader, sizeof(buffer), NULL, buffer);
		die("Failed to compile %s shader: %s", type == GL_VERTEX_SHADER ? "vertex" : "fragment", buffer);
	}

	return shader;
}

void ortho2d(float m[4][4], float left, float right, float bottom, float top)
{
	m[0][0] = 1; m[0][1] = 0; m[0][2] = 0; m[0][3] = 0;
	m[1][0] = 0; m[1][1] = 1; m[1][2] = 0; m[1][3] = 0;
	m[2][0] = 0; m[2][1] = 0; m[2][2] = 1; m[2][3] = 0;
	m[3][0] = 0; m[3][1] = 0; m[3][2] = 0; m[3][3] = 1;

	m[0][0] = 2.0f / (right - left);
	m[1][1] = 2.0f / (top - bottom);
	m[2][2] = -1.0f;
	m[3][0] = -(right + left) / (right - left);
	m[3][1] = -(top + bottom) / (top - bottom);
}

static void init_shaders()
{
	GLuint vshader = compile_shader(GL_VERTEX_SHADER, 1, &vshader_default_src);
	GLuint fshader = 0;
	if (strcmp(g_cfg.shader, "zfast-crt") == 0)
		fshader = compile_shader(GL_FRAGMENT_SHADER, 1, &fshader_zfastcrt_src);
	else if (strcmp(g_cfg.shader, "zfast-lcd") == 0)
		fshader = compile_shader(GL_FRAGMENT_SHADER, 1, &fshader_zfastlcd_src);
	else
		fshader = compile_shader(GL_FRAGMENT_SHADER, 1, &fshader_default_src);
	GLuint program = glCreateProgram();

	assert(program);

	glAttachShader(program, vshader);
	glAttachShader(program, fshader);
	glLinkProgram(program);

	glDeleteShader(vshader);
	glDeleteShader(fshader);

	glValidateProgram(program);

	GLint status;
	glGetProgramiv(program, GL_LINK_STATUS, &status);

	if (status == GL_FALSE)
	{
		char buffer[4096];
		glGetProgramInfoLog(program, sizeof(buffer), NULL, buffer);
		die("Failed to link shader program: %s", buffer);
	}

	shader.program    = program;
	shader.i_pos      = glGetAttribLocation(program,  "i_pos");
	shader.i_coord    = glGetAttribLocation(program,  "i_coord");
	shader.u_tex      = glGetUniformLocation(program, "u_tex");
	shader.u_tex_size = glGetUniformLocation(program, "u_tex_size");
	shader.u_mvp      = glGetUniformLocation(program, "u_mvp");

	glGenVertexArrays(1, &shader.vao);
	glGenBuffers(1, &shader.vbo);

	glUseProgram(shader.program);

	glUniform1i(shader.u_tex, 0);

	float m[4][4];
	if (video.hw.bottom_left_origin)
		ortho2d(m, -1, 1, 1, -1);
	else
		ortho2d(m, -1, 1, -1, 1);

	glUniformMatrix4fv(shader.u_mvp, 1, GL_FALSE, (float*)m);

	glUseProgram(0);
}

void create_window(int width, int height)
{
	if (video.hw.context_type == RETRO_HW_CONTEXT_OPENGL_CORE || video.hw.version_major >= 3) {
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, video.hw.version_major);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, video.hw.version_minor);
	}
	else
	{
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
	}

	switch (video.hw.context_type) {
		case RETRO_HW_CONTEXT_OPENGL_CORE:
			glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
			break;
		case RETRO_HW_CONTEXT_OPENGLES2:
			glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
			break;
		case RETRO_HW_CONTEXT_OPENGL:
			if (video.hw.version_major >= 3)
				glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);
			break;
		default:
			die("Unsupported hw context %i. (only OPENGL, OPENGL_CORE and OPENGLES2 supported)", video.hw.context_type);
	}

	GLFWmonitor* monitor = NULL;
	if (g_cfg.fullscreen)
	{
		monitor = glfwGetPrimaryMonitor();
		const GLFWvidmode *mode = glfwGetVideoMode(monitor);
		width = mode->width;
		height = mode->height;
	}

	window = glfwCreateWindow(width, height, g_cfg.title, monitor, NULL);

	if (!window)
		die("Failed to create window.");

	if (g_cfg.hide_cursor)
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);

	glfwMakeContextCurrent(window);

	// if (video.hw.context_type == RETRO_HW_CONTEXT_OPENGLES2) {
	// 	if (!gladLoadGLES2Loader((GLADloadproc)glfwGetProcAddress))
	// 		die("Failed to initialize glad.");
	// } else {
		if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
			die("Failed to initialize glad.");
	// }

	init_shaders();

	glfwSwapInterval(1);

	glEnable(GL_TEXTURE_2D);
}

static void refresh_vertex_data()
{
	assert(video.tex_w);
	assert(video.tex_h);
	assert(video.clip_w);
	assert(video.clip_h);

	float bottom = (float)video.clip_h / video.tex_h;
	float right  = (float)video.clip_w / video.tex_w;

	float vertex_data[] = {
		// pos, coord
		-1.0f, -1.0f, 0.0f,  bottom, // left-bottom
		-1.0f,  1.0f, 0.0f,  0.0f,   // left-top
		 1.0f, -1.0f, right,  bottom,// right-bottom
		 1.0f,  1.0f, right,  0.0f,  // right-top
	};

	glBindVertexArray(shader.vao);

	glBindBuffer(GL_ARRAY_BUFFER, shader.vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_data), vertex_data, GL_STREAM_DRAW);

	glEnableVertexAttribArray(shader.i_pos);
	glEnableVertexAttribArray(shader.i_coord);
	glVertexAttribPointer(shader.i_pos, 2, GL_FLOAT, GL_FALSE, sizeof(float)*4, 0);
	glVertexAttribPointer(shader.i_coord, 2, GL_FLOAT, GL_FALSE, sizeof(float)*4, (void*)(2 * sizeof(float)));

	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

static void init_framebuffer(int width, int height)
{
	glGenFramebuffers(1, &video.fbo_id);
	glBindFramebuffer(GL_FRAMEBUFFER, video.fbo_id);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, video.tex_id, 0);

	if (video.hw.depth && video.hw.stencil)
	{
		glGenRenderbuffers(1, &video.rbo_id);
		glBindRenderbuffer(GL_RENDERBUFFER, video.rbo_id);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);

		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, video.rbo_id);
	}
	else if (video.hw.depth)
	{
		glGenRenderbuffers(1, &video.rbo_id);
		glBindRenderbuffer(GL_RENDERBUFFER, video.rbo_id);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);

		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, video.rbo_id);
	}

	if (video.hw.depth || video.hw.stencil)
		glBindRenderbuffer(GL_RENDERBUFFER, 0);

	assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);

	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

static void resize_to_aspect(double ratio, int sw, int sh, int *dw, int *dh)
{
	*dw = sw;
	*dh = sh;

	if (ratio <= 0)
		ratio = (double)sw / sh;

	if ((float)sw / sh < 1)
		*dw = *dh * ratio;
	else
		*dh = *dw / ratio;
}

uintptr_t video_get_current_framebuffer()
{
	return video.fbo_id;
}

void video_set_hw(struct retro_hw_render_callback hw)
{
	video.hw = hw;
}

static void noop() {}

void video_configure(const struct retro_game_geometry *geom)
{
	int nwidth, nheight;

	resize_to_aspect(geom->aspect_ratio, geom->base_width * 1, geom->base_height * 1, &nwidth, &nheight);

	nwidth *= g_cfg.scale;
	nheight *= g_cfg.scale;

	video.hw.version_major   = 2;
	video.hw.version_minor   = 1;
	video.hw.context_type    = RETRO_HW_CONTEXT_OPENGL;
	video.hw.context_reset   = noop;
	video.hw.context_destroy = noop;

	if (!window)
		create_window(nwidth, nheight);

	video.tex_id = 0;

	if (!video.pixfmt)
	{
		video.pixfmt = GL_UNSIGNED_SHORT_5_5_5_1;
		video.pixtype = GL_BGRA;
		video.bpp = 2;
	}

	glGenTextures(1, &video.tex_id);

	if (!video.tex_id)
		die("Failed to create the video texture");

	video.pitch = geom->base_width * video.bpp;

	glBindTexture(GL_TEXTURE_2D, video.tex_id);

	int filter = GL_NEAREST;
	if (strcmp(g_cfg.filter, "linear") == 0)
		filter = GL_LINEAR;

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, geom->max_width, geom->max_height, 0,
			video.pixtype, video.pixfmt, NULL);

	glBindTexture(GL_TEXTURE_2D, 0);

	init_framebuffer(geom->max_width, geom->max_height);

	video.tex_w = geom->max_width;
	video.tex_h = geom->max_height;
	video.clip_w = geom->base_width;
	video.clip_h = geom->base_height;

	refresh_vertex_data();

	video.hw.context_reset();
}

void video_set_geometry(const struct retro_game_geometry *geom)
{
	video.tex_w = geom->max_width;
	video.tex_h = geom->max_height;
	video.clip_w = geom->base_width;
	video.clip_h = geom->base_height;
	printf("Set geom %dx%d\n", video.clip_w, video.clip_h);

	if (window) {
		refresh_vertex_data();

		int ow = 0, oh = 0;
		resize_to_aspect(geom->aspect_ratio, geom->base_width, geom->base_height, &ow, &oh);

		// ow *= g_cfg.scale;
		// oh *= g_cfg.scale;

		// if (!g_cfg.fullscreen)
		// 	glfwSetWindowSize(window, ow, oh);
	}
}

bool video_set_pixel_format(unsigned format)
{
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

	if (video.pitch)
		glPixelStorei(GL_UNPACK_ROW_LENGTH, video.pitch / video.bpp);

	return true;
}

void video_refresh(const void *data, unsigned width, unsigned height, size_t pitch)
{
	video.clip_h = height;
	video.clip_w = width;
	video.pitch = pitch;
	refresh_vertex_data();

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindTexture(GL_TEXTURE_2D, video.tex_id);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, video.pitch / video.bpp);

	if (data)
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, video.pixtype, video.pixfmt, data);
}

void video_render()
{
	int w = 0, h = 0;
	glfwGetFramebufferSize(window, &w, &h);
	glViewport(0, 0, w, h);

	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT);

	glUseProgram(shader.program);
	glUniform2f(shader.u_tex_size, (float)video.tex_w, (float)video.tex_h);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, video.tex_id);

	glBindVertexArray(shader.vao);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glBindVertexArray(0);

	glUseProgram(0);
}

void video_deinit()
{
	if (video.fbo_id)
		glDeleteFramebuffers(1, &video.fbo_id);

	if (video.tex_id)
		glDeleteTextures(1, &video.tex_id);

	if (shader.vao)
		glDeleteVertexArrays(1, &shader.vao);

	if (shader.vbo)
		glDeleteBuffers(1, &shader.vbo);

	if (shader.program)
		glDeleteProgram(shader.program);

	glfwDestroyWindow(window);
}
