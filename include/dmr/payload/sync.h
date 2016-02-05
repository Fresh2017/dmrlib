/**
 * @file
 * @brief Payload SYNC data.
 */
#ifndef _DMR_PAYLOAD_SYNC_H
#define _DMR_PAYLOAD_SYNC_H

#include <inttypes.h>
#include <dmr/packet.h>

typedef enum {
    DMR_SYNC_PATTERN_BS_SOURCED_VOICE = 0x00,
    DMR_SYNC_PATTERN_BS_SOURCED_DATA,
    DMR_SYNC_PATTERN_MS_SOURCED_VOICE,
    DMR_SYNC_PATTERN_MS_SOURCED_DATA,
    DMR_SYNC_PATTERN_MS_SOURCED_RC,
    DMR_SYNC_PATTERN_DIRECT_VOICE_TS1,
    DMR_SYNC_PATTERN_DIRECT_DATA_TS1,
    DMR_SYNC_PATTERN_DIRECT_VOICE_TS2,
    DMR_SYNC_PATTERN_DIRECT_DATA_TS2,
    DMR_SYNC_PATTERN_UNKNOWN
} dmr_sync_pattern;

extern dmr_sync_pattern dmr_sync_pattern_decode(dmr_packet packet);
extern int              dmr_sync_pattern_encode(dmr_packet packet, dmr_sync_pattern pattern);
extern char *           dmr_sync_pattern_name(dmr_sync_pattern sync_pattern);

#endif // _DMR_PAYLOAD_SYNC_H
