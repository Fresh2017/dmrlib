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

typedef struct {
    dmr_proto_t  proto;
    mbe_parms    curr_mp;
    mbe_parms    prev_mp;
    mbe_parms    prev_mp_enhanced;
    uint8_t      quality;
    dmr_thread_t *thread;
    bool         active;
} dmr_mbe_t;

extern dmr_mbe_t *dmr_mbe_new(uint8_t quality);
extern dmr_decoded_voice_t *dmr_mbe_decode_frame(dmr_mbe_t *mbe, dmr_ambe_frame_bits_t *frame);
extern void dmr_mbe_decode(dmr_mbe_t *mbe, dmr_voice_t *voice, float *samples);
extern void dmr_mbe_free(dmr_mbe_t *mbe);

#endif // DMR_PLATFORM_WINDOWS

#endif // _DMR_PROTO_MBE_H
