#ifndef _DMR_PAYLOAD_H
#define _DMR_PAYLOAD_H

#include <dmr/bits.h>
#include <dmr/type.h>

#define DMR_PAYLOAD_BYTES    33
#define DMR_PAYLOAD_BITS     (DMR_PAYLOAD_BYTES*8)

#define DMR_PAYLOAD_SLOT_TYPE_HALF   10
#define DMR_PAYLOAD_SLOT_TYPE_BITS   (DMR_PAYLOAD_SLOT_TYPE_HALF*2)

typedef struct {
    uint8_t bytes[DMR_PAYLOAD_BYTES];
    bit_t   bits[DMR_PAYLOAD_BITS]; // See DMR AI spec. page 85.
} dmr_payload_t;


typedef struct {
	bit_t bits[72];
} dmr_payload_ambe_frame_bits_t;

typedef union {
	struct {
		bit_t bits[108*2];
	} raw;
	struct {
		dmr_payload_ambe_frame_bits_t frames[3];
	} ambe_frames;
} dmr_payload_voice_bits_t;

typedef struct {
	uint8_t bytes[sizeof(dmr_payload_voice_bits_t)/8];
} dmr_payload_voice_bytes_t;

#include <dmr/payload/csbk.h>
#include <dmr/payload/emb.h>
#include <dmr/payload/info.h>
#include <dmr/payload/sync.h>

#endif // _DMR_PAYLOAD_H
