/**
 * @file
 */
#ifndef _DMR_PAYLOAD_H
#define _DMR_PAYLOAD_H

#ifdef __cplusplus
extern "C" {
#endif

#include <dmr/bits.h>
#include <dmr/type.h>

typedef struct {
    uint8_t bytes[DMR_PACKET_LEN];
    bool    bits[DMR_PACKET_BITS]; // See DMR AI spec. page 85.
} dmr_payload;

typedef struct {
	bool bits[72];
} dmr_payload_ambe_frame_bits;

typedef union {
	struct {
		bool bits[108*2];
	} raw;
	struct {
		dmr_payload_ambe_frame_bits frames[3];
	} ambe_frames;
} dmr_payload_voice_bits;

typedef struct {
	uint8_t bytes[sizeof(dmr_payload_voice_bits) / 8];
} dmr_payload_voice_bytes;

#ifdef __cplusplus
}
#endif

#endif // _DMR_PAYLOAD_H
