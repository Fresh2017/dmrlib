#ifndef _DMR_PAYLOAD_SYNC_H
#define _DMR_PAYLOAD_SYNC_H

#include <dmr/payload/info.h>

#define DMR_PAYLOAD_SYNC_BYTES  6
#define DMR_PAYLOAD_SYNC_BITS   (DMR_PAYLOAD_SYNC_BYTES*8)
#define DMR_PAYLOAD_SYNC_OFFSET (DMR_PAYLOAD_INFO_HALF+DMR_PAYLOAD_SLOT_TYPE_HALF)

#define DMR_PAYLOAD_SYNC_PATTERN_UNKNOWN		  	  0x00
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

#endif // _DMR_PAYLOAD_SYNC_H
