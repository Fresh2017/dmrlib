#ifndef _NOISEBRIDGE_AUDIO_H
#define _NOISEBRIDGE_AUDIO_H

#include "config.h"

void stream_audio(float *samples, size_t len);
void kill_audio(void);
bool boot_audio(void);
bool init_audio(void);

#endif // _NOISEBRIDGE_AUDIO_H
