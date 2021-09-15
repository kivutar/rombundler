#include <assert.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/glad.h>

#include "libretro.h"
#include "config.h"
#include "utils.h"

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
} video = {0};

static struct {
	GLuint vao;
	GLuint vbo;
	GLuint program;

	GLint i_pos;
	GLint i_coord;
	GLint u_tex;
	GLint u_mvp;

} g_shader = {0};

static const char *g_vshader_src =
	"attribute vec2 i_pos;\n"
	"attribute vec2 i_coord;\n"
	"varying vec2 o_coord;\n"
	"uniform mat4 u_mvp;\n"
	"void main() {\n"
		"o_coord = i_coord;\n"
		"gl_Position = vec4(i_pos, 0.0, 1.0) * u_mvp;\n"
	"}";

static const char *g_fshader_src =
	"varying vec2 o_coord;\n"
	"uniform sampler2D u_tex;\n"
	"void main() {\n"
		"gl_FragColor = texture2D(u_tex, o_coord);\n"
	"}";

static GLuint compile_shader(unsigned type, unsigned count, const char **strings) {
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

void ortho2d(float m[4][4], float left, float right, float bottom, float top) {
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

static void init_shaders() {
	GLuint vshader = compile_shader(GL_VERTEX_SHADER, 1, &g_vshader_src);
	GLuint fshader = compile_shader(GL_FRAGMENT_SHADER, 1, &g_fshader_src);
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

	if(status == GL_FALSE) {
		char buffer[4096];
		glGetProgramInfoLog(program, sizeof(buffer), NULL, buffer);
		die("Failed to link shader program: %s", buffer);
	}

	g_shader.program = program;
	g_shader.i_pos   = glGetAttribLocation(program,  "i_pos");
	g_shader.i_coord = glGetAttribLocation(program,  "i_coord");
	g_shader.u_tex   = glGetUniformLocation(program, "u_tex");
	g_shader.u_mvp   = glGetUniformLocation(program, "u_mvp");

	glGenVertexArraysAPPLE(1, &g_shader.vao);
	glGenBuffers(1, &g_shader.vbo);

	glUseProgram(g_shader.program);

	glUniform1i(g_shader.u_tex, 0);

	float m[4][4];
	// if (video.hw.bottom_left_origin)
	// 	ortho2d(m, -1, 1, 1, -1);
	// else
		ortho2d(m, -1, 1, -1, 1);

	glUniformMatrix4fv(g_shader.u_mvp, 1, GL_FALSE, (float*)m);

	glUseProgram(0);
}

static void resize_cb(GLFWwindow *win, int w, int h) {
	glViewport(0, 0, w, h);
}

void create_window(int width, int height) {
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
	//glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

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

	glfwSetFramebufferSizeCallback(window, resize_cb);

	glfwMakeContextCurrent(window);

	if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress))
		die("Failed to initialize glad");

	init_shaders();

	glfwSwapInterval(1);

	glEnable(GL_TEXTURE_2D);

	resize_cb(window, width, height);
}

static void refresh_vertex_data() {
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

	glBindVertexArrayAPPLE(g_shader.vao);

	glBindBuffer(GL_ARRAY_BUFFER, g_shader.vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_data), vertex_data, GL_STREAM_DRAW);

	glEnableVertexAttribArray(g_shader.i_pos);
	glEnableVertexAttribArray(g_shader.i_coord);
	glVertexAttribPointer(g_shader.i_pos, 2, GL_FLOAT, GL_FALSE, sizeof(float)*4, 0);
	glVertexAttribPointer(g_shader.i_coord, 2, GL_FLOAT, GL_FALSE, sizeof(float)*4, (void*)(2 * sizeof(float)));

	glBindVertexArrayAPPLE(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

static void init_framebuffer(int width, int height)
{
	glGenFramebuffers(1, &video.fbo_id);
	glBindFramebuffer(GL_FRAMEBUFFER, video.fbo_id);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, video.tex_id, 0);

	/*f (video.hw.depth && video.hw.stencil) {
		glGenRenderbuffers(1, &video.rbo_id);
		glBindRenderbuffer(GL_RENDERBUFFER, video.rbo_id);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);

		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, video.rbo_id);
	} else if (video.hw.depth) {
		glGenRenderbuffers(1, &video.rbo_id);
		glBindRenderbuffer(GL_RENDERBUFFER, video.rbo_id);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);

		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, video.rbo_id);
	}

	if (video.hw.depth || video.hw.stencil)
		glBindRenderbuffer(GL_RENDERBUFFER, 0);*/

	glBindRenderbuffer(GL_RENDERBUFFER, 0);

	assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);

	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
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

	video.tex_id = 0;

	if (!video.pixfmt)
		video.pixfmt = GL_UNSIGNED_SHORT_5_5_5_1;

	//glfwSetWindowSize(window, nwidth, nheight);

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

	init_framebuffer(geom->max_width, geom->max_height);

	video.tex_w = geom->max_width;
	video.tex_h = geom->max_height;
	video.clip_w = geom->base_width;
	video.clip_h = geom->base_height;

	refresh_vertex_data();

	//video.hw.context_reset();
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

void video_refresh(const void *data, unsigned width, unsigned height, size_t pitch) {
	if (video.clip_w != width || video.clip_h != height) {
		video.clip_h = height;
		video.clip_w = width;

		refresh_vertex_data();
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindTexture(GL_TEXTURE_2D, video.tex_id);

	if (pitch != video.pitch) {
		video.pitch = pitch;
	}

	if (data)
	{
		glPixelStorei(GL_UNPACK_ROW_LENGTH, video.pitch / video.bpp);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, video.pixtype, video.pixfmt, data);
	}
}

void video_render() {
	int w = 0, h = 0;
	glfwGetFramebufferSize(window, &w, &h);
	glViewport(0, 0, w, h);

	glClear(GL_COLOR_BUFFER_BIT);

	glUseProgram(g_shader.program);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, video.tex_id);

	glBindVertexArrayAPPLE(g_shader.vao);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glBindVertexArrayAPPLE(0);

	glUseProgram(0);
}

void video_deinit() {
	if (video.fbo_id)
		glDeleteFramebuffers(1, &video.fbo_id);

	if (video.tex_id)
		glDeleteTextures(1, &video.tex_id);

	if (g_shader.vao)
		glDeleteVertexArraysAPPLE(1, &g_shader.vao);

	if (g_shader.vbo)
		glDeleteBuffers(1, &g_shader.vbo);

	if (g_shader.program)
		glDeleteProgram(g_shader.program);

	glfwDestroyWindow(window);
}
