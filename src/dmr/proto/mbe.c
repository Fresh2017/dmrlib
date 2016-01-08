#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>

#include "dmr/platform.h"
#include "dmr/proto.h"
#include "dmr/proto/mbe.h"
#include "dmr/payload/voice.h"

#ifdef DMR_PLATFORM_WINDOWS
// TODO See if mbelib can be ported to win32
#else

static const char *dmr_mbe_proto_name = "mbe";

static uint8_t mbe_decode_deinterleave_matrix_w[36] = {
    0x00, 0x01, 0x00, 0x01, 0x00, 0x01,
    0x00, 0x01, 0x00, 0x01, 0x00, 0x01,
    0x00, 0x01, 0x00, 0x01, 0x00, 0x01,
    0x00, 0x01, 0x00, 0x01, 0x00, 0x02,
    0x00, 0x02, 0x00, 0x02, 0x00, 0x02,
    0x00, 0x02, 0x00, 0x02, 0x00, 0x02
};

static uint8_t mbe_decode_deinterleave_matrix_x[36] = {
    0x17, 0x0a, 0x16, 0x09, 0x15, 0x08,
    0x14, 0x07, 0x13, 0x06, 0x12, 0x05,
    0x11, 0x04, 0x10, 0x03, 0x0f, 0x02,
    0x0e, 0x01, 0x0d, 0x00, 0x0c, 0x0a,
    0x0b, 0x09, 0x0a, 0x08, 0x09, 0x07,
    0x08, 0x06, 0x07, 0x05, 0x06, 0x04
};

static uint8_t mbe_decode_deinterleave_matrix_y[36] = {
    0x00, 0x02, 0x00, 0x02, 0x00, 0x02,
    0x00, 0x02, 0x00, 0x03, 0x00, 0x03,
    0x01, 0x03, 0x01, 0x03, 0x01, 0x03,
    0x01, 0x03, 0x01, 0x03, 0x01, 0x03,
    0x01, 0x03, 0x01, 0x03, 0x01, 0x03,
    0x01, 0x03, 0x01, 0x03, 0x01, 0x03
};

static uint8_t mbe_decode_deinterleave_matrix_z[36] = {
    0x05, 0x03, 0x04, 0x02, 0x03, 0x01,
    0x02, 0x00, 0x01, 0x0d, 0x00, 0x0c,
    0x16, 0x0b, 0x15, 0x0a, 0x14, 0x09,
    0x13, 0x08, 0x12, 0x07, 0x11, 0x06,
    0x10, 0x05, 0x0f, 0x04, 0x0e, 0x03,
    0x0d, 0x02, 0x0c, 0x01, 0x0b, 0x00
};

dmr_mbe_t *dmr_mbe_new(uint8_t quality)
{
    dmr_mbe_t *mbe = malloc(sizeof(dmr_mbe_t));
    if (mbe == NULL)
        return NULL;

    mbe->proto.name = dmr_mbe_proto_name;
    mbe->quality = quality;
    mbe_initMbeParms(
        &mbe->curr_mp,
        &mbe->prev_mp,
        &mbe->prev_mp_enhanced
    );

    return mbe;
}

dmr_decoded_voice_t *dmr_mbe_decode_frame(dmr_mbe_t *mbe, dmr_ambe_frame_bits_t *frame)
{
    static dmr_decoded_voice_t decoded;
    char deinterleaved_bits[4][24];
    uint8_t j;
    uint8_t *w, *x, *y, *z;
    int errs, errs2;
    char err_str[64];
    char ambe_d[49];

    if (mbe == NULL || frame == NULL)
        return NULL;

    // Deinterleaving
    w = mbe_decode_deinterleave_matrix_w;
    x = mbe_decode_deinterleave_matrix_x;
    y = mbe_decode_deinterleave_matrix_y;
    z = mbe_decode_deinterleave_matrix_z;

    for (j = 0; j < sizeof(dmr_ambe_frame_bits_t); j += 2) {
        deinterleaved_bits[*w][*x] = frame->bits[j];
        deinterleaved_bits[*y][*z] = frame->bits[j+1];
        w++; x++; y++; z++;
    }

    mbe_processAmbe3600x2450Framef(
        decoded.samples,
        &errs, &errs2, err_str,
        deinterleaved_bits, ambe_d,
        &mbe->curr_mp,
        &mbe->prev_mp,
        &mbe->prev_mp_enhanced,
        mbe->quality);

    if (errs2 > 0)
        fprintf(stderr, "mbe: %u decoding errors: %s\n", errs2, err_str);

    return &decoded;
}

void dmr_mbe_decode(dmr_mbe_t *mbe, dmr_voice_t *voice, float *samples)
{
    dmr_decoded_voice_t *decoded;
    int i, j;

    if (mbe == NULL || voice == NULL || samples == NULL)
        return;

    for (i = 0; i < 3; i++) {
        decoded = dmr_mbe_decode_frame(mbe, &voice->frames.frame[i]);
        if (decoded == NULL)
            return;

        for (j = 0; j < DMR_DECODED_AMBE_FRAME_SAMPLES; j++) {
            samples[j + (i * DMR_DECODED_AMBE_FRAME_SAMPLES)] = decoded->samples[j];
        }
    }
}

void dmr_mbe_free(dmr_mbe_t *mbe)
{
    if (mbe == NULL)
        return;

    free(mbe);
}

static dmr_proto_status_t dmr_proto_mbe_init(void *mbeptr)
{
    if (mbeptr == NULL)
        return DMR_PROTO_CONF;
    else
        return DMR_PROTO_OK;
}

static void mbe_process_voice(void *mbeptr, dmr_voice_t *voice, float *samples)
{
    dmr_mbe_t *mbe = (dmr_mbe_t *)mbeptr;

    if (mbe == NULL || voice == NULL || samples == NULL)
        return;

    dmr_mbe_decode(mbe, voice, samples);
}

#endif // DMR_PLATFORM_WINDOWS
