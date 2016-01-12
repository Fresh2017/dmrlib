#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>

#include "dmr/log.h"
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

static int mbe_proto_init(void *mbeptr)
{
    dmr_log_trace("mbe: init %p", mbeptr);
    if (mbeptr == NULL)
        return dmr_error(DMR_EINVAL);
    return 0;
}

static int mbe_proto_start(void *mbeptr)
{
    dmr_log_trace("mbe: start %p", mbeptr);
    dmr_mbe_t *mbe = (dmr_mbe_t *)mbeptr;
    if (mbe == NULL)
        return dmr_error(DMR_EINVAL);
    if (mbe->samples_cb == NULL) {
        dmr_log_error("mbe: no samples callback");
        return dmr_error(DMR_EINVAL);
    }
    return 0;
}

static void mbe_proto_process_voice(void *mbeptr, dmr_voice_bits_t *voice, float *samples)
{
    dmr_log_trace("mbe: process voice %p", mbeptr);
    dmr_mbe_t *mbe = (dmr_mbe_t *)mbeptr;

    if (mbe == NULL || voice == NULL || samples == NULL)
        return;

    dmr_mbe_decode(mbe, voice, samples);
}

static void mbe_proto_tx(void *mbeptr, dmr_packet_t *packet)
{
    dmr_log_trace("mbe: tx %p", mbeptr);
    dmr_mbe_t *mbe = (dmr_mbe_t *)mbeptr;
    dmr_voice_bits_t voice_bits;

    if (mbe == NULL || packet == NULL)
        return;

    dmr_log_debug("mbe: tx %s", dmr_slot_type_name(packet->slot_type));
    switch (packet->slot_type) {
    case DMR_SLOT_TYPE_VOICE_BURST_A:
    case DMR_SLOT_TYPE_VOICE_BURST_B:
    case DMR_SLOT_TYPE_VOICE_BURST_C:
    case DMR_SLOT_TYPE_VOICE_BURST_D:
    case DMR_SLOT_TYPE_VOICE_BURST_E:
    case DMR_SLOT_TYPE_VOICE_BURST_F:
        break;
    default:
        dmr_log_debug("mbe: ignored unsupported packet");
        return;
    }

    if (dmr_payload_bits(packet, &voice_bits) != 0) {
        dmr_log_error("mbe: unable to extract voice bits: %s", dmr_error_get());
        return;
    }

    int i;
    dmr_decoded_voice_t *frame;
    for (i = 0; i < 3; i++) {
        if ((frame = dmr_mbe_decode_frame(mbe, &voice_bits.frames.frame[i])) == NULL) {
            dmr_log_error("mbe: unable to decode voice frame");
            return;
        }
    }
}

dmr_mbe_t *dmr_mbe_new(uint8_t quality, dmr_mbe_samples_cb_t samples_cb)
{
    dmr_mbe_t *mbe = malloc(sizeof(dmr_mbe_t));
    if (mbe == NULL)
        return NULL;

    // Setup protocol
    mbe->proto.name = dmr_mbe_proto_name;
    mbe->proto.type = DMR_PROTO_MBE;
    mbe->proto.init = mbe_proto_init;
    mbe->proto.start = mbe_proto_start;
    //mbe->proto.process_voice = mbe_proto_process_voice;
    mbe->proto.tx = mbe_proto_tx;

    mbe->quality = quality;
    mbe->samples_cb = samples_cb;
    mbe_initMbeParms(
        &mbe->curr_mp,
        &mbe->prev_mp,
        &mbe->prev_mp_enhanced
    );

    return mbe;
}

dmr_decoded_voice_t *dmr_mbe_decode_frame(dmr_mbe_t *mbe, dmr_voice_frame_t *frame)
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

    for (j = 0; j < sizeof(dmr_voice_frame_t); j += 2) {
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

void dmr_mbe_decode(dmr_mbe_t *mbe, dmr_voice_bits_t *voice, float *samples)
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

#endif // DMR_PLATFORM_WINDOWS
