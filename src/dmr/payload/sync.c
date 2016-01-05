#include <string.h>
#include <stdio.h>

#include "dmr/payload.h"

static uint8_t dmr_payload_sync_pattern_bs_sourced_voice[6] = { 0x75, 0x5F, 0xD7, 0xDF, 0x75, 0xF7 };
static uint8_t dmr_payload_sync_pattern_bs_sourced_data[6]  = { 0xDF, 0xF5, 0x7D, 0x75, 0xDF, 0x5D };
static uint8_t dmr_payload_sync_pattern_ms_sourced_voice[6] = { 0x7F, 0x7D, 0x5D, 0xD5, 0x7D, 0xFD };
static uint8_t dmr_payload_sync_pattern_ms_sourced_data[6]  = { 0xD5, 0xD7, 0xF7, 0x7F, 0xD7, 0x57 };
static uint8_t dmr_payload_sync_pattern_ms_sourced_rc[6]    = { 0x77, 0xD5, 0x5F, 0x7D, 0xFD, 0x77 };
static uint8_t dmr_payload_sync_pattern_direct_voice_ts1[6] = { 0x5D, 0x57, 0x7F, 0x77, 0x57, 0xFF };
static uint8_t dmr_payload_sync_pattern_direct_data_ts1[6]  = { 0xF7, 0xFD, 0xD5, 0xDD, 0xFD, 0x55 };
static uint8_t dmr_payload_sync_pattern_direct_voice_ts2[6] = { 0x7D, 0xFF, 0xD5, 0xF5, 0x5D, 0x5F };
static uint8_t dmr_payload_sync_pattern_direct_data_ts2[6]  = { 0xD7, 0x55, 0x7F, 0x5F, 0xF7, 0xF5 };

dmr_payload_sync_bits_t *dmr_payload_get_sync_bits(dmr_payload_t *payload)
{
    static dmr_payload_sync_bits_t sync_bits;
    memcpy(&sync_bits.bits, &payload->bits[DMR_PAYLOAD_SYNC_OFFSET], DMR_PAYLOAD_SYNC_BITS);
    return &sync_bits;
}

dmr_payload_sync_pattern_t dmr_sync_bits_get_sync_pattern(dmr_payload_sync_bits_t *sync_bits)
{
    uint8_t sync_bytes[6];
    bits_to_bytes(&sync_bits->bits[0], DMR_PAYLOAD_SYNC_BITS,
                  &sync_bytes[0], DMR_PAYLOAD_SYNC_BYTES);

    if (!memcmp(sync_bytes, dmr_payload_sync_pattern_bs_sourced_voice, sizeof(sync_bytes)))
        return DMR_PAYLOAD_SYNC_PATTERN_BS_SOURCED_VOICE;
    else if (!memcmp(sync_bytes, dmr_payload_sync_pattern_bs_sourced_data, sizeof(sync_bytes)))
        return DMR_PAYLOAD_SYNC_PATTERN_BS_SOURCED_DATA;
    else if (!memcmp(sync_bytes, dmr_payload_sync_pattern_ms_sourced_voice, sizeof(sync_bytes)))
        return DMR_PAYLOAD_SYNC_PATTERN_MS_SOURCED_VOICE;
    else if (!memcmp(sync_bytes, dmr_payload_sync_pattern_ms_sourced_data, sizeof(sync_bytes)))
        return DMR_PAYLOAD_SYNC_PATTERN_MS_SOURCED_DATA;
    else if (!memcmp(sync_bytes, dmr_payload_sync_pattern_ms_sourced_rc, sizeof(sync_bytes)))
        return DMR_PAYLOAD_SYNC_PATTERN_MS_SOURCED_RC;
    else if (!memcmp(sync_bytes, dmr_payload_sync_pattern_direct_voice_ts1, sizeof(sync_bytes)))
        return DMR_PAYLOAD_SYNC_PATTERN_DIRECT_VOICE_TS1;
    else if (!memcmp(sync_bytes, dmr_payload_sync_pattern_direct_data_ts1, sizeof(sync_bytes)))
        return DMR_PAYLOAD_SYNC_PATTERN_DIRECT_DATA_TS1;
    else if (!memcmp(sync_bytes, dmr_payload_sync_pattern_direct_voice_ts2, sizeof(sync_bytes)))
        return DMR_PAYLOAD_SYNC_PATTERN_DIRECT_VOICE_TS2;
    else if (!memcmp(sync_bytes, dmr_payload_sync_pattern_direct_data_ts2, sizeof(sync_bytes)))
        return DMR_PAYLOAD_SYNC_PATTERN_DIRECT_DATA_TS2;
    else
        return DMR_PAYLOAD_SYNC_PATTERN_UNKNOWN;
}

char *dmr_payload_get_sync_pattern_name(dmr_payload_sync_pattern_t sync_pattern)
{
    switch (sync_pattern) {
    case DMR_PAYLOAD_SYNC_PATTERN_BS_SOURCED_VOICE:
        return "bs sourced voice";
    case DMR_PAYLOAD_SYNC_PATTERN_BS_SOURCED_DATA:
        return "bs sourced data";
    case DMR_PAYLOAD_SYNC_PATTERN_MS_SOURCED_VOICE:
        return "ms sourced voice";
    case DMR_PAYLOAD_SYNC_PATTERN_MS_SOURCED_DATA:
        return "ms sourced data";
    case DMR_PAYLOAD_SYNC_PATTERN_MS_SOURCED_RC:
        return "ms sourced rc";
    case DMR_PAYLOAD_SYNC_PATTERN_DIRECT_VOICE_TS1:
        return "direct voice ts1";
    case DMR_PAYLOAD_SYNC_PATTERN_DIRECT_DATA_TS1:
        return "direct data ts1";
    case DMR_PAYLOAD_SYNC_PATTERN_DIRECT_VOICE_TS2:
        return "direct voice ts2";
    case DMR_PAYLOAD_SYNC_PATTERN_DIRECT_DATA_TS2:
        return "direct data ts2";
    case DMR_PAYLOAD_SYNC_PATTERN_UNKNOWN:
    default:
        return "unknown";
    }
}

void dmr_payload_put_sync_pattern(dmr_payload_t *payload, dmr_payload_sync_pattern_t pattern)
{
    static dmr_payload_sync_bits_t sync_bits;

    if (payload == NULL)
        return;

    switch (pattern) {
    case DMR_PAYLOAD_SYNC_PATTERN_BS_SOURCED_VOICE:
        bytes_to_bits(dmr_payload_sync_pattern_bs_sourced_voice, 6, sync_bits.bits, sizeof(dmr_payload_sync_bits_t));
        break;
    case DMR_PAYLOAD_SYNC_PATTERN_BS_SOURCED_DATA:
        bytes_to_bits(dmr_payload_sync_pattern_bs_sourced_data, 6, sync_bits.bits, sizeof(dmr_payload_sync_bits_t));
        break;
    case DMR_PAYLOAD_SYNC_PATTERN_MS_SOURCED_VOICE:
        bytes_to_bits(dmr_payload_sync_pattern_ms_sourced_voice, 6, sync_bits.bits, sizeof(dmr_payload_sync_bits_t));
        break;
    case DMR_PAYLOAD_SYNC_PATTERN_MS_SOURCED_DATA:
        bytes_to_bits(dmr_payload_sync_pattern_ms_sourced_data, 6, sync_bits.bits, sizeof(dmr_payload_sync_bits_t));
        break;
    case DMR_PAYLOAD_SYNC_PATTERN_MS_SOURCED_RC:
        bytes_to_bits(dmr_payload_sync_pattern_ms_sourced_rc, 6, sync_bits.bits, sizeof(dmr_payload_sync_bits_t));
        break;
    case DMR_PAYLOAD_SYNC_PATTERN_DIRECT_VOICE_TS1:
        bytes_to_bits(dmr_payload_sync_pattern_direct_voice_ts1, 6, sync_bits.bits, sizeof(dmr_payload_sync_bits_t));
        break;
    case DMR_PAYLOAD_SYNC_PATTERN_DIRECT_DATA_TS1:
        bytes_to_bits(dmr_payload_sync_pattern_direct_data_ts1, 6, sync_bits.bits, sizeof(dmr_payload_sync_bits_t));
        break;
    case DMR_PAYLOAD_SYNC_PATTERN_DIRECT_VOICE_TS2:
        bytes_to_bits(dmr_payload_sync_pattern_direct_voice_ts2, 6, sync_bits.bits, sizeof(dmr_payload_sync_bits_t));
        break;
    case DMR_PAYLOAD_SYNC_PATTERN_DIRECT_DATA_TS2:
        bytes_to_bits(dmr_payload_sync_pattern_direct_data_ts2, 6, sync_bits.bits, sizeof(dmr_payload_sync_bits_t));
        break;
    case DMR_PAYLOAD_SYNC_PATTERN_UNKNOWN:
    default:
        memset(sync_bits.bits, 0, sizeof(dmr_payload_sync_bits_t));
        return;
    }

    memcpy(&payload->bits[DMR_PAYLOAD_SYNC_OFFSET], &sync_bits.bits[0], DMR_PAYLOAD_SYNC_BITS);
}
