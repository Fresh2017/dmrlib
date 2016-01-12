/**
 * @file
 * @brief MBElib protocol.
 */
#ifndef _DMR_PROTO_MBE_H
#define _DMR_PROTO_MBE_H

#include <dmr/platform.h>

#ifdef DMR_PLATFORM_WINDOWS
#warning mbelib is not supported on Windows
#else

#include <mbelib.h>

#include <dmr/proto.h>
#include <dmr/thread.h>

typedef void (*dmr_mbe_samples_cb_t)(float*, size_t);

typedef struct {
    dmr_proto_t          proto;
    mbe_parms            curr_mp;
    mbe_parms            prev_mp;
    mbe_parms            prev_mp_enhanced;
    uint8_t              quality;
    dmr_mbe_samples_cb_t samples_cb;
    dmr_thread_t         *thread;
} dmr_mbe_t;

extern dmr_mbe_t *dmr_mbe_new(uint8_t quality, dmr_mbe_samples_cb_t samples_cb);
extern dmr_decoded_voice_t *dmr_mbe_decode_frame(dmr_mbe_t *mbe, dmr_voice_frame_t *frame);
extern void dmr_mbe_decode(dmr_mbe_t *mbe, dmr_voice_bits_t *voice, float *samples);
extern void dmr_mbe_free(dmr_mbe_t *mbe);

#endif // DMR_PLATFORM_WINDOWS

#endif // _DMR_PROTO_MBE_H
