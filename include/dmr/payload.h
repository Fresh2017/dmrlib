#ifndef _DMR_PAYLOAD_H
#define _DMR_PAYLOAD_H

#include <dmr/bits.h>
#include <dmr/type.h>

#define DMR_PAYLOAD_BYTES    33
#define DMR_PAYLOAD_BITS     (DMR_PAYLOAD_BYTES*8)

#define DMR_PAYLOAD_INFO_HALF        98
#define DMR_PAYLOAD_INFO_BITS        (DMR_PAYLOAD_INFO_HALF*2)
#define DMR_PAYLOAD_SLOT_TYPE_HALF   10
#define DMR_PAYLOAD_SLOT_TYPE_BITS   (DMR_PAYLOAD_SLOT_TYPE_HALF*2)
#define DMR_PAYLOAD_SYNC_BYTES       6
#define DMR_PAYLOAD_SYNC_BITS        (DMR_PAYLOAD_SYNC_BYTES*8)
#define DMR_PAYLOAD_SYNC_OFFSET      (DMR_PAYLOAD_INFO_HALF+DMR_PAYLOAD_SLOT_TYPE_HALF)

typedef struct {
    uint8_t bytes[DMR_PAYLOAD_BYTES];
    bit_t   bits[DMR_PAYLOAD_BITS]; // See DMR AI spec. page 85.
} dmr_payload_t;

typedef struct {
	bit_t bits[DMR_PAYLOAD_INFO_BITS];
} dmr_payload_info_bits_t;

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

#define DMR_PAYLOAD_SYNC_PATTERN_UNKNOWN			0x00
#define DMR_PAYLOAD_SYNC_PATTERN_BS_SOURCED_VOICE	0x01
#define DMR_PAYLOAD_SYNC_PATTERN_BS_SOURCED_DATA	0x02
#define DMR_PAYLOAD_SYNC_PATTERN_MS_SOURCED_VOICE	0x03
#define DMR_PAYLOAD_SYNC_PATTERN_MS_SOURCED_DATA	0x04
#define DMR_PAYLOAD_SYNC_PATTERN_MS_SOURCED_RC		0x05
#define DMR_PAYLOAD_SYNC_PATTERN_DIRECT_VOICE_TS1	0x06
#define DMR_PAYLOAD_SYNC_PATTERN_DIRECT_DATA_TS1	0x07
#define DMR_PAYLOAD_SYNC_PATTERN_DIRECT_VOICE_TS2	0x08
#define DMR_PAYLOAD_SYNC_PATTERN_DIRECT_DATA_TS2	0x09
typedef uint8_t dmr_payload_sync_pattern_t;

typedef struct {
    bit_t bits[DMR_PAYLOAD_SYNC_BITS];
} dmr_payload_sync_bits_t;

extern dmr_payload_sync_bits_t *dmr_payload_get_sync_bits(dmr_payload_t *payload);
extern dmr_payload_sync_pattern_t dmr_sync_bits_get_sync_pattern(dmr_payload_sync_bits_t *sync_bits);
extern char *dmr_payload_get_sync_pattern_name(dmr_payload_sync_pattern_t sync_pattern);
extern void dmr_payload_put_sync_pattern(dmr_payload_t *payload, dmr_payload_sync_pattern_t pattern);

#define DMR_EMB_LCSS_SINGLE_FRAGMENT                0b00
#define DMR_EMB_LCSS_FIRST_FRAGMENT                 0b01
#define DMR_EMB_LCSS_LAST_FRAGMENT                  0b10
#define DMR_EMB_LCSS_CONTINUATION                   0b11
typedef uint8_t dmr_emb_lcss_t;

typedef struct {
    dmr_color_code_t color_code;
    dmr_emb_lcss_t lcss;
} dmr_emb_t;

typedef struct {
    bit_t bits[16];
} dmr_emb_bits_t;

typedef struct {
	bit_t bits[72];
	bit_t checksum[5];
} dmr_emb_signalling_lc_bits_t;

typedef struct {
    bit_t bits[32];
} dmr_emb_signalling_lc_fragment_bits_t;

extern dmr_emb_bits_t *dmr_emb_from_payload_sync_bits(dmr_payload_sync_bits_t *sync_bits);
extern dmr_emb_t *dmr_emb_decode(dmr_emb_bits_t *emb_bits);
extern char *dmr_emb_lcss_name(dmr_emb_lcss_t lcss);

#endif // _DMR_PAYLOAD_H
