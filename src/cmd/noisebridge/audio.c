#include <stdlib.h>
#include <string.h>
#include <portaudio.h>
#include <dmr/config.h>
#include <dmr/bits.h>
#include <dmr/payload/voice.h>
#include "audio.h"

#if defined(WITH_MBELIB) && defined(HAVE_LIBPORTAUDIO)

static PaStream *stream;
static unsigned long stream_pos, stream_write_pos = 0;
static float samples[DMR_DECODED_AMBE_FRAME_SAMPLES] = { 0 };
static PaStreamParameters output = {
    -1,
    1,
    paFloat32,
    0,
    NULL
};

void stream_audio(float *samples, size_t len)
{
    unsigned long out = 0, cur;
    while (out < len) {
        cur = min(DMR_DECODED_AMBE_FRAME_SAMPLES - stream_write_pos, len);
        memcpy(&samples[stream_pos], &samples[out], cur);
        stream_write_pos = (stream_write_pos + cur) % DMR_DECODED_AMBE_FRAME_SAMPLES;
        out += cur;
    }
}

//void stream_audio_cb(void *mem, uint8_t *stream, int len)
int stream_audio_cb(const void *input, void *output, unsigned long len, const PaStreamCallbackTimeInfo *timeInfo, PaStreamCallbackFlags statusFlags, void *mem)
{
    DMR_UNUSED(input);
    DMR_UNUSED(timeInfo);
    DMR_UNUSED(statusFlags);
    DMR_UNUSED(mem);

    unsigned long out = 0, cur;
    while (out < len) {
        cur = min(DMR_DECODED_AMBE_FRAME_SAMPLES - stream_pos, len);
        memcpy(output, &samples[stream_pos], cur);
        memset(&samples[stream_pos], 0, cur);
        stream_pos = (stream_pos + cur) % DMR_DECODED_AMBE_FRAME_SAMPLES;
        out += cur;
    }

    return paContinue;
}

void kill_audio(void)
{
    int err;

    if ((err = Pa_Terminate()) != paNoError) {
        dmr_log_critical("audio: kill failed: %s", Pa_GetErrorText(err));
    }
}

bool boot_audio(void)
{
    int err;

    if ((err =  Pa_OpenDefaultStream(&stream, 0, 1, paFloat32, 8000, DMR_DECODED_AMBE_FRAME_SAMPLES, stream_audio_cb, NULL)) != paNoError) {
        dmr_log_critical("audio: init failed: %s", Pa_GetErrorText(err));
        return false;
    }

    return true;
}

bool init_audio(void)
{
    int err, i, n;
    const PaDeviceInfo *dev;
    config_t *config = load_config();

    dmr_log_info("audio: %s device",
        config->audio_device == NULL ? "detecting" : "configuring");

    if ((err = Pa_Initialize()) != paNoError) {
        dmr_log_critical("audio: boot failed: %s", Pa_GetErrorText(err));
        return false;
    }
    //atexit(kill_audio);

    n = Pa_GetDeviceCount();
    if (n < 0) {
        dmr_log_critical("audio: Pa_CountDevices returned 0x%x", n);
        return false;
    }
    if (n == 0) {
        dmr_log_critical("audio: no audio devices available");
        return false;
    }

    output.device = -1;
    for (i = 0; i < n; i++) {
        dev = Pa_GetDeviceInfo(i);
        if (dev->maxOutputChannels <= 0)
            continue;
        dmr_log_info("audio: device %d: \"%s\"", i, dev->name);

        if (config->audio_device == NULL && output.device == -1) {
            dmr_log_info("audio: auto selected device, use \"audio_device\" to override");
            output.device = i;
            output.suggestedLatency = dev->defaultLowOutputLatency;
        } else if (config->audio_device != NULL && !strcmp(config->audio_device, dev->name)) {
            output.device = i;
            output.suggestedLatency = dev->defaultLowOutputLatency;
        }
    }

    if (output.device == -1) {
        if (config->audio_device != NULL)
            dmr_log_critical("audio: audio device \"%s\" not available", config->audio_device);
        else
            dmr_log_critical("audio: no audio devices available");
        return false;
    }

    return true;
}

#else // WITH_MBELIB && HAVE_LIBPORTAUDIO

bool init_audio(void)
{
    return true;
}

bool boot_audio(void)
{
    return true;
}

void kill_audio(void)
{
}

void stream_audio(float *samples, size_t len)
{
    DMR_UNUSED(samples);
    DMR_UNUSED(len);
}

#endif // WITH_MBELIB && HAVE_LIBPORTAUDIO
