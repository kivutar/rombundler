#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>

#ifdef __APPLE__
#include <OpenAL/al.h>
#include <OpenAL/alc.h>
#else
#include <AL/al.h>
#include <AL/alc.h>
#endif

#define BUFSIZE 1024*8
#define NUMBUFFERS 4

#ifndef MIN
#define MIN(x,y)	((x) <= (y) ? (x) : (y))
#endif

#include "audio.h"
#include "utils.h"

typedef struct al
{
	ALuint source;
	ALuint *buffers;
	ALuint *res_buf;
	ALCdevice *handle;
	ALCcontext *ctx;
	size_t res_ptr;
	size_t tmpbuf_ptr;
	int rate;
	uint8_t tmpbuf[BUFSIZE];
} al_t;

al_t* al = NULL;

static bool unqueue_buffers()
{
	ALint val;

	alGetSourcei(al->source, AL_BUFFERS_PROCESSED, &val);

	if (val <= 0)
		return false;

	alSourceUnqueueBuffers(al->source, val, &al->res_buf[al->res_ptr]);
	al->res_ptr += val;
	return true;
}

static bool get_buffer(ALuint *buffer)
{
	if (!al->res_ptr)
	{
		for (;;)
		{
			if (unqueue_buffers())
				break;
		}
	}

	*buffer = al->res_buf[--al->res_ptr];
	return true;
}

static size_t fill_internal_buf(const void *buf, size_t size)
{
	size_t read_size = MIN(BUFSIZE - al->tmpbuf_ptr, size);
	memcpy(al->tmpbuf + al->tmpbuf_ptr, buf, read_size);
	al->tmpbuf_ptr += read_size;
	return read_size;
}

size_t audio_write(const void *buf_, unsigned size) {
	if (!al || !buf_ || !size)
		return size;

	const uint8_t *buf = (const uint8_t*)buf_;
	size_t written = 0;

	while (size)
	{
		ALint val;
		ALuint buffer;
		size_t rc = fill_internal_buf(buf, size);

		written += rc;
		buf += rc;
		size -= rc;

		if (al->tmpbuf_ptr != BUFSIZE)
			break;

		if (!get_buffer(&buffer))
			break;

		alBufferData(buffer, AL_FORMAT_STEREO16, al->tmpbuf, BUFSIZE, al->rate);
		al->tmpbuf_ptr = 0;
		alSourceQueueBuffers(al->source, 1, &buffer);
		if (alGetError() != AL_NO_ERROR)
			return -1;

		alGetSourcei(al->source, AL_SOURCE_STATE, &val);
		if (val != AL_PLAYING)
			alSourcePlay(al->source);

		if (alGetError() != AL_NO_ERROR)
			return -1;
	}

	return written;
}

void audio_deinit() {
	if (!al)
		return;

	alSourceStop(al->source);
	alDeleteSources(1, &al->source);

	if (al->buffers)
		alDeleteBuffers(NUMBUFFERS, al->buffers);

	free(al->buffers);
	free(al->res_buf);
	alcMakeContextCurrent(NULL);

	if (al->ctx)
		alcDestroyContext(al->ctx);
	if (al->handle)
		alcCloseDevice(al->handle);
	free(al);
}

void audio_init(int rate) {
	al = (al_t*)calloc(1, sizeof(al_t));
	if (!al)
		return;

	al->handle = alcOpenDevice(NULL);
	if (!al->handle)
		die("Could not init audio handle");

	al->ctx = alcCreateContext(al->handle, NULL);
	if (!al->ctx)
		die("Could not init audio context");

	alcMakeContextCurrent(al->ctx);

	al->rate = rate;
	al->buffers = (ALuint*)calloc(NUMBUFFERS, sizeof(ALuint));
	al->res_buf = (ALuint*)calloc(NUMBUFFERS, sizeof(ALuint));
	if (!al->buffers || !al->res_buf)
		die("Could not init audio buffers");

	alGenSources(1, &al->source);
	alGenBuffers(NUMBUFFERS, al->buffers);

	memcpy(al->res_buf, al->buffers, NUMBUFFERS * sizeof(ALuint));
	al->res_ptr = NUMBUFFERS;
}

void audio_sample(int16_t left, int16_t right) {
	int16_t buf[2] = {left, right};
	audio_write(buf, 4);
}

size_t audio_sample_batch(const int16_t *data, size_t frames) {
	return audio_write(data, frames*4);
}
