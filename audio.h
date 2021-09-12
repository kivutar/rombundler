#include <stdio.h>

void audio_init(int frequency);
void audio_deinit();
void audio_sample(int16_t left, int16_t right);
size_t audio_sample_batch(const int16_t *data, size_t frames);
