#ifndef _DMR_PAYLOAD_SYNC_H
#define _DMR_PAYLOAD_SYNC_H

#include <dmr/payload/info.h>

#define DMR_PAYLOAD_SYNC_BYTES  6
#define DMR_PAYLOAD_SYNC_BITS   (DMR_PAYLOAD_SYNC_BYTES*8)
#define DMR_PAYLOAD_SYNC_OFFSET (DMR_PAYLOAD_INFO_HALF+DMR_PAYLOAD_SLOT_TYPE_HALF)

#define DMR_PAYLOAD_SYNC_PATTERN_UNKNOWN			0b0000
#define DMR_PAYLOAD_SYNC_PATTERN_BS_SOURCED_VOICE	0b0001
#define DMR_PAYLOAD_SYNC_PATTERN_BS_SOURCED_DATA	0b0010
#define DMR_PAYLOAD_SYNC_PATTERN_MS_SOURCED_VOICE	0b0011
#define DMR_PAYLOAD_SYNC_PATTERN_MS_SOURCED_DATA	0b0100
#define DMR_PAYLOAD_SYNC_PATTERN_MS_SOURCED_RC		0b0101
#define DMR_PAYLOAD_SYNC_PATTERN_DIRECT_VOICE_TS1	0b0110
#define DMR_PAYLOAD_SYNC_PATTERN_DIRECT_DATA_TS1	0b0111
#define DMR_PAYLOAD_SYNC_PATTERN_DIRECT_VOICE_TS2	0b1000
#define DMR_PAYLOAD_SYNC_PATTERN_DIRECT_DATA_TS2	0b1001
typedef uint8_t dmr_payload_sync_pattern_t;

typedef struct {
    bit_t bits[DMR_PAYLOAD_SYNC_BITS];
} dmr_payload_sync_bits_t;

extern dmr_payload_sync_bits_t *dmr_payload_get_sync_bits(dmr_payload_t *payload);
extern dmr_payload_sync_pattern_t dmr_sync_bits_get_sync_pattern(dmr_payload_sync_bits_t *sync_bits);
extern char *dmr_payload_get_sync_pattern_name(dmr_payload_sync_pattern_t sync_pattern);
extern void dmr_payload_put_sync_pattern(dmr_payload_t *payload, dmr_payload_sync_pattern_t pattern);

#endif // _DMR_PAYLOAD_SYNC_H
