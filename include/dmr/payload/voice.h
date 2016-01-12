/**
 * @file
 * @brief Voice payload.
 */
#ifndef _DMR_VOICE_H
#define _DMR_VOICE_H

#include <dmr/bits.h>
#include <dmr/packet.h>

#define DMR_DECODED_AMBE_FRAME_SAMPLES 160

typedef struct {
    bool bits[72];
} dmr_voice_frame_t;

typedef union {
    struct {
        bool bits[108 * 2];
    } raw;
    struct {
        dmr_voice_frame_t frame[3];
    } frames;
} dmr_voice_bits_t;

typedef struct {
    float samples[DMR_DECODED_AMBE_FRAME_SAMPLES];
} dmr_decoded_voice_t;

#endif // _DMR_VOICE_H
