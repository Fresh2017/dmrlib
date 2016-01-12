#include <string.h>
#include <stdio.h>

#include "dmr/payload/sync.h"

static uint8_t dmr_payload_sync_pattern_bs_sourced_voice[6] = { 0x75, 0x5f, 0xd7, 0xdf, 0x75, 0xf7 };
static uint8_t dmr_payload_sync_pattern_bs_sourced_data[6]  = { 0xdf, 0xf5, 0x7d, 0x75, 0xdf, 0x5d };
static uint8_t dmr_payload_sync_pattern_ms_sourced_voice[6] = { 0x7f, 0x7d, 0x5d, 0xd5, 0x7d, 0xfd };
static uint8_t dmr_payload_sync_pattern_ms_sourced_data[6]  = { 0xd5, 0xd7, 0xf7, 0x7f, 0xd7, 0x57 };
static uint8_t dmr_payload_sync_pattern_ms_sourced_rc[6]    = { 0x77, 0xd5, 0x5f, 0x7d, 0xfd, 0x77 };
static uint8_t dmr_payload_sync_pattern_direct_voice_ts1[6] = { 0x5d, 0x57, 0x7f, 0x77, 0x57, 0xff };
static uint8_t dmr_payload_sync_pattern_direct_data_ts1[6]  = { 0xf7, 0xfd, 0xd5, 0xdd, 0xfd, 0x55 };
static uint8_t dmr_payload_sync_pattern_direct_voice_ts2[6] = { 0x7d, 0xff, 0xd5, 0xf5, 0x5d, 0x5f };
static uint8_t dmr_payload_sync_pattern_direct_data_ts2[6]  = { 0xd7, 0x55, 0x7f, 0x5f, 0xf7, 0xf5 };

dmr_sync_pattern_t dmr_sync_pattern_decode(dmr_packet_t *packet)
{
    uint8_t sync_bytes[6], i;
    for (i = 0; i < 6; i++) {
        sync_bytes[i]  = (packet->payload[17 + i] & 0x0f) << 4;
        sync_bytes[i] |= (packet->payload[18 + i] & 0xf0) >> 4;
    }

    if (!memcmp(sync_bytes, dmr_payload_sync_pattern_bs_sourced_voice, sizeof(sync_bytes)))
        return DMR_SYNC_PATTERN_BS_SOURCED_VOICE;
    else if (!memcmp(sync_bytes, dmr_payload_sync_pattern_bs_sourced_data, sizeof(sync_bytes)))
        return DMR_SYNC_PATTERN_BS_SOURCED_DATA;
    else if (!memcmp(sync_bytes, dmr_payload_sync_pattern_ms_sourced_voice, sizeof(sync_bytes)))
        return DMR_SYNC_PATTERN_MS_SOURCED_VOICE;
    else if (!memcmp(sync_bytes, dmr_payload_sync_pattern_ms_sourced_data, sizeof(sync_bytes)))
        return DMR_SYNC_PATTERN_MS_SOURCED_DATA;
    else if (!memcmp(sync_bytes, dmr_payload_sync_pattern_ms_sourced_rc, sizeof(sync_bytes)))
        return DMR_SYNC_PATTERN_MS_SOURCED_RC;
    else if (!memcmp(sync_bytes, dmr_payload_sync_pattern_direct_voice_ts1, sizeof(sync_bytes)))
        return DMR_SYNC_PATTERN_DIRECT_VOICE_TS1;
    else if (!memcmp(sync_bytes, dmr_payload_sync_pattern_direct_data_ts1, sizeof(sync_bytes)))
        return DMR_SYNC_PATTERN_DIRECT_DATA_TS1;
    else if (!memcmp(sync_bytes, dmr_payload_sync_pattern_direct_voice_ts2, sizeof(sync_bytes)))
        return DMR_SYNC_PATTERN_DIRECT_VOICE_TS2;
    else if (!memcmp(sync_bytes, dmr_payload_sync_pattern_direct_data_ts2, sizeof(sync_bytes)))
        return DMR_SYNC_PATTERN_DIRECT_DATA_TS2;
    else
        return DMR_SYNC_PATTERN_UNKNOWN;
}

char *dmr_sync_pattern_name(dmr_sync_pattern_t sync_pattern)
{
    switch (sync_pattern) {
    case DMR_SYNC_PATTERN_BS_SOURCED_VOICE:
        return "bs sourced voice";
    case DMR_SYNC_PATTERN_BS_SOURCED_DATA:
        return "bs sourced data";
    case DMR_SYNC_PATTERN_MS_SOURCED_VOICE:
        return "ms sourced voice";
    case DMR_SYNC_PATTERN_MS_SOURCED_DATA:
        return "ms sourced data";
    case DMR_SYNC_PATTERN_MS_SOURCED_RC:
        return "ms sourced rc";
    case DMR_SYNC_PATTERN_DIRECT_VOICE_TS1:
        return "direct voice ts1";
    case DMR_SYNC_PATTERN_DIRECT_DATA_TS1:
        return "direct data ts1";
    case DMR_SYNC_PATTERN_DIRECT_VOICE_TS2:
        return "direct voice ts2";
    case DMR_SYNC_PATTERN_DIRECT_DATA_TS2:
        return "direct data ts2";
    case DMR_SYNC_PATTERN_UNKNOWN:
    default:
        return "unknown";
    }
}

int dmr_sync_pattern_encode(dmr_sync_pattern_t pattern, dmr_packet_t *packet)
{
    if (packet == NULL)
        return -1;

    uint8_t sync_bytes[6], i;
    switch (pattern) {
    case DMR_SYNC_PATTERN_BS_SOURCED_VOICE:
        memcpy(sync_bytes, dmr_payload_sync_pattern_bs_sourced_voice, sizeof(sync_bytes));
        break;
    case DMR_SYNC_PATTERN_BS_SOURCED_DATA:
        memcpy(sync_bytes, dmr_payload_sync_pattern_bs_sourced_data, sizeof(sync_bytes));
        break;
    case DMR_SYNC_PATTERN_MS_SOURCED_VOICE:
        memcpy(sync_bytes, dmr_payload_sync_pattern_ms_sourced_voice, sizeof(sync_bytes));
        break;
    case DMR_SYNC_PATTERN_MS_SOURCED_DATA:
        memcpy(sync_bytes, dmr_payload_sync_pattern_ms_sourced_data, sizeof(sync_bytes));
        break;
    case DMR_SYNC_PATTERN_MS_SOURCED_RC:
        memcpy(sync_bytes, dmr_payload_sync_pattern_ms_sourced_rc, sizeof(sync_bytes));
        break;
    case DMR_SYNC_PATTERN_DIRECT_VOICE_TS1:
        memcpy(sync_bytes, dmr_payload_sync_pattern_direct_voice_ts1, sizeof(sync_bytes));
        break;
    case DMR_SYNC_PATTERN_DIRECT_DATA_TS1:
        memcpy(sync_bytes, dmr_payload_sync_pattern_direct_data_ts1, sizeof(sync_bytes));
        break;
    case DMR_SYNC_PATTERN_DIRECT_VOICE_TS2:
        memcpy(sync_bytes, dmr_payload_sync_pattern_direct_voice_ts2, sizeof(sync_bytes));
        break;
    case DMR_SYNC_PATTERN_DIRECT_DATA_TS2:
        memcpy(sync_bytes, dmr_payload_sync_pattern_direct_data_ts2, sizeof(sync_bytes));
        break;
    case DMR_SYNC_PATTERN_UNKNOWN:
    default:
        return -1;
    }

    for (i = 0; i < 6; i++) {
        packet->payload[17 + i] |= ((sync_bytes[i] >> 4) & 0x0f);
        packet->payload[18 + i] |= ((sync_bytes[i] << 4) & 0xf0);
    }
    return 0;
}
