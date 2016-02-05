#include <string.h>
#include <stdio.h>

#include "dmr/payload/sync.h"

static const uint8_t dmr_sync_pattern_bs_sourced_voice[] = { 0x07, 0x55, 0xFD, 0x7D, 0xF7, 0x5F, 0x70 };
static const uint8_t dmr_sync_pattern_bs_sourced_data[]  = { 0x0D, 0xFF, 0x57, 0xD7, 0x5D, 0xF5, 0xD0 };
static const uint8_t dmr_sync_pattern_ms_sourced_voice[] = { 0x07, 0xF7, 0xD5, 0xDD, 0x57, 0xDF, 0xD0 };
static const uint8_t dmr_sync_pattern_ms_sourced_data[]  = { 0x0D, 0x5D, 0x7F, 0x77, 0xFD, 0x75, 0x70 };
static const uint8_t dmr_sync_pattern_ms_sourced_rc[]    = { 0x07, 0x7D, 0x55, 0xF7, 0xDF, 0xD7, 0x70 };
static const uint8_t dmr_sync_pattern_direct_voice_ts1[] = { 0x05, 0xD5, 0x77, 0xF7, 0x75, 0x7F, 0xF0 };
static const uint8_t dmr_sync_pattern_direct_data_ts1[]  = { 0x0F, 0x7F, 0xDD, 0x5D, 0xDF, 0xD5, 0x50 };
static const uint8_t dmr_sync_pattern_direct_voice_ts2[] = { 0x07, 0xDF, 0xFD, 0x5F, 0x55, 0xD5, 0xF0 };
static const uint8_t dmr_sync_pattern_direct_data_ts2[]  = { 0x0D, 0x75, 0x57, 0xF5, 0xFF, 0x7F, 0x50 };
static const uint8_t dmr_sync_pattern_mask[]             = { 0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF0 };
static const uint8_t dmr_sync_delta_max = 4;
static const uint8_t dmr_sync_bits[] = {
    0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4,
    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
    3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
    4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8
};

uint8_t dmr_bit_diff(const uint8_t *a, const uint8_t *b)
{
    uint8_t delta = 0, i, j;
    for (i = 0; i < 7; i++) {
        j = dmr_sync_pattern_mask[i] & (a[i] ^ b[i]);
        delta += dmr_sync_bits[j];
    }
    return delta;
}

dmr_sync_pattern dmr_sync_pattern_decode(dmr_packet packet)
{
    const uint8_t *buf = (const uint8_t *)(packet + 13);

    if (dmr_bit_diff(buf, dmr_sync_pattern_bs_sourced_voice) < dmr_sync_delta_max)
        return DMR_SYNC_PATTERN_BS_SOURCED_VOICE;
    if (dmr_bit_diff(buf, dmr_sync_pattern_bs_sourced_data) < dmr_sync_delta_max)
        return DMR_SYNC_PATTERN_BS_SOURCED_DATA;
    if (dmr_bit_diff(buf, dmr_sync_pattern_ms_sourced_voice) < dmr_sync_delta_max)
        return DMR_SYNC_PATTERN_MS_SOURCED_VOICE;
    if (dmr_bit_diff(buf, dmr_sync_pattern_ms_sourced_data) < dmr_sync_delta_max)
        return DMR_SYNC_PATTERN_MS_SOURCED_DATA;
    if (dmr_bit_diff(buf, dmr_sync_pattern_ms_sourced_rc) < dmr_sync_delta_max)
        return DMR_SYNC_PATTERN_MS_SOURCED_RC;
    if (dmr_bit_diff(buf, dmr_sync_pattern_direct_voice_ts1) < dmr_sync_delta_max)
        return DMR_SYNC_PATTERN_DIRECT_VOICE_TS1;
    if (dmr_bit_diff(buf, dmr_sync_pattern_direct_data_ts1) < dmr_sync_delta_max)
        return DMR_SYNC_PATTERN_DIRECT_DATA_TS1;
    if (dmr_bit_diff(buf, dmr_sync_pattern_direct_voice_ts2) < dmr_sync_delta_max)
        return DMR_SYNC_PATTERN_DIRECT_VOICE_TS2;
    if (dmr_bit_diff(buf, dmr_sync_pattern_direct_data_ts2) < dmr_sync_delta_max)
        return DMR_SYNC_PATTERN_DIRECT_DATA_TS2;

    return DMR_SYNC_PATTERN_UNKNOWN;
}

char *dmr_sync_pattern_name(dmr_sync_pattern sync_pattern)
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

int dmr_sync_pattern_encode(dmr_packet packet, dmr_sync_pattern pattern)
{
    if (packet == NULL)
        return -1;

    const uint8_t *sync_pattern;
    switch (pattern) {
    case DMR_SYNC_PATTERN_BS_SOURCED_VOICE:
        sync_pattern = dmr_sync_pattern_bs_sourced_voice;
        break;
    case DMR_SYNC_PATTERN_BS_SOURCED_DATA:
        sync_pattern = dmr_sync_pattern_bs_sourced_data;
        break;
    case DMR_SYNC_PATTERN_MS_SOURCED_VOICE:
        sync_pattern = dmr_sync_pattern_ms_sourced_voice;
        break;
    case DMR_SYNC_PATTERN_MS_SOURCED_DATA:
        sync_pattern = dmr_sync_pattern_ms_sourced_data;
        break;
    case DMR_SYNC_PATTERN_MS_SOURCED_RC:
        sync_pattern = dmr_sync_pattern_ms_sourced_rc;
        break;
    case DMR_SYNC_PATTERN_DIRECT_VOICE_TS1:
        sync_pattern = dmr_sync_pattern_direct_voice_ts1;
        break;
    case DMR_SYNC_PATTERN_DIRECT_DATA_TS1:
        sync_pattern = dmr_sync_pattern_direct_data_ts1;
        break;
    case DMR_SYNC_PATTERN_DIRECT_VOICE_TS2:
        sync_pattern = dmr_sync_pattern_direct_voice_ts2;
        break;
    case DMR_SYNC_PATTERN_DIRECT_DATA_TS2:
        sync_pattern = &dmr_sync_pattern_direct_data_ts2[0];
        break;
    case DMR_SYNC_PATTERN_UNKNOWN:
    default:
        return -1;
    }

    uint8_t i;
    for (i = 0; i < 7; i++) {
        packet[i + 13] = (packet[i + 13] & ~dmr_sync_pattern_mask[i]) | sync_pattern[i];
    }
    return 0;
}
