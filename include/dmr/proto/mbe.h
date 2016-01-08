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
    mbe_parms    curr_mp;
    mbe_parms    prev_mp;
    mbe_parms    prev_mp_enhanced;
    uint8_t      quality;
    dmr_thread_t *thread;
    bool         active;
    dmr_proto_t  proto;
} dmr_mbe_t;

#endif // DMR_PLATFORM_WINDOWS

#endif // _DMR_PROTO_MBE_H
