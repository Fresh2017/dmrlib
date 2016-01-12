/**
 * @file
 * @brief Payload SYNC data.
 */
#ifndef _DMR_PAYLOAD_SYNC_H
#define _DMR_PAYLOAD_SYNC_H

#include <inttypes.h>
#include <dmr/packet.h>

typedef enum {
    DMR_SYNC_PATTERN_UNKNOWN,
    DMR_SYNC_PATTERN_BS_SOURCED_VOICE,
    DMR_SYNC_PATTERN_BS_SOURCED_DATA,
    DMR_SYNC_PATTERN_MS_SOURCED_VOICE,
    DMR_SYNC_PATTERN_MS_SOURCED_DATA,
    DMR_SYNC_PATTERN_MS_SOURCED_RC,
    DMR_SYNC_PATTERN_DIRECT_VOICE_TS1,
    DMR_SYNC_PATTERN_DIRECT_DATA_TS1,
    DMR_SYNC_PATTERN_DIRECT_VOICE_TS2,
    DMR_SYNC_PATTERN_DIRECT_DATA_TS2
} dmr_sync_pattern_t;

extern dmr_sync_pattern_t dmr_sync_pattern_decode(dmr_packet_t *packet);
extern int dmr_sync_pattern_encode(dmr_sync_pattern_t pattern, dmr_packet_t *packet);
extern char *dmr_sync_pattern_name(dmr_sync_pattern_t sync_pattern);

#endif // _DMR_PAYLOAD_SYNC_H
