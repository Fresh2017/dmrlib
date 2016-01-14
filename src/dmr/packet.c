#include <stdlib.h>
#include <string.h>
#include "dmr/bits.h"
#include "dmr/error.h"
#include "dmr/log.h"
#include "dmr/packet.h"
#include "dmr/fec/golay_20_8.h"

static char *flco_name[] = {
    "group",
    "unit to unit",
    "invalid",
    NULL
};

static struct {
    dmr_fid_t fid;
    char      *name;
} fid_name[] = {
    { DMR_FID_ETSI, "ETSI"            },
    { DMR_FID_DMRA, "DMR Association" },
    { 0,            NULL              }
};

static char *ts_name[] = {
    "TS1",
    "TS2",
    "invalid",
    NULL
};

dmr_packet_t *dmr_packet_decode(uint8_t *buf, size_t len)
{
    dmr_packet_t *packet;
    if (len != DMR_PAYLOAD_BYTES) {
        dmr_log_error("dmr: can't decode packet of %d bytes, need %d", len, DMR_PAYLOAD_BYTES);
        return NULL;
    }

    packet = malloc(sizeof(dmr_packet_t));
    if (packet == NULL) {
        dmr_error(DMR_ENOMEM);
        return NULL;
    }
    memset(packet, 0, sizeof(dmr_packet_t));
    memcpy(packet->payload, buf, DMR_PAYLOAD_BYTES);
    return packet;
}

int dmr_payload_bits(dmr_packet_t *packet, void *bits)
{
    if (packet == NULL || bits == NULL)
        return dmr_error(DMR_EINVAL);

    bool data[33 * 8];
    dmr_bytes_to_bits(packet->payload, 33, data, sizeof(data));
    memcpy(bits + 0 , &data[0],   98);
    memcpy(bits + 98, &data[166], 98);
    return 0;
}

char *dmr_fid_name(dmr_fid_t fid)
{
    uint8_t i;
    for (i = 0; fid_name[i].name != NULL; i++) {
        if (fid_name[i].fid == fid)
            return fid_name[i].name;
    }
    return NULL;
}

char *dmr_flco_name(dmr_flco_t flco)
{
    flco = min(flco, DMR_FLCO_INVALID);
    return flco_name[flco];
}

char *dmr_ts_name(dmr_ts_t ts)
{
    ts = min(ts, DMR_TS_INVALID);
    return ts_name[ts];
}

char *dmr_data_type_name(dmr_data_type_t data_type)
{
    switch (data_type) {
    case DMR_DATA_TYPE_VOICE_PI:
        return "privacy indicator";
    case DMR_DATA_TYPE_VOICE_LC:
        return "voice lc";
    case DMR_DATA_TYPE_TERMINATOR_WITH_LC:
        return "terminator with lc";
    case DMR_DATA_TYPE_CSBK:
        return "csbk";
    case DMR_DATA_TYPE_MBC:
        return "multi block control";
    case DMR_DATA_TYPE_MBCC:
        return "multi block control continuation";
    case DMR_DATA_TYPE_DATA:
        return "data";
    case DMR_DATA_TYPE_RATE12_DATA:
        return "rate 1/2 data";
    case DMR_DATA_TYPE_RATE34_DATA:
        return "rate 3/4 data";
    case DMR_DATA_TYPE_IDLE:
        return "idle";
    case DMR_DATA_TYPE_VOICE:
        return "voice";
    case DMR_DATA_TYPE_VOICE_SYNC:
        return "voice sync";
    case DMR_DATA_TYPE_INVALID:
    default:
        return "invalid";
    }
}

char *dmr_data_type_name_short(dmr_data_type_t data_type)
{
    switch (data_type) {
    case DMR_DATA_TYPE_VOICE_PI:
        return "voice pi";
    case DMR_DATA_TYPE_VOICE_LC:
        return "voice lc";
    case DMR_DATA_TYPE_TERMINATOR_WITH_LC:
        return "term lc";
    case DMR_DATA_TYPE_CSBK:
        return "csbk";
    case DMR_DATA_TYPE_MBC:
        return "mbc";
    case DMR_DATA_TYPE_MBCC:
        return "mbcc";
    case DMR_DATA_TYPE_DATA:
        return "data";
    case DMR_DATA_TYPE_RATE12_DATA:
        return "r 1/2";
    case DMR_DATA_TYPE_RATE34_DATA:
        return "r 3/4";
    case DMR_DATA_TYPE_IDLE:
        return "idle";
    case DMR_DATA_TYPE_VOICE:
        return "voice";
    case DMR_DATA_TYPE_VOICE_SYNC:
        return "vc sync";
    case DMR_DATA_TYPE_INVALID:
    default:
        return "invalid";
    }
}

int dmr_slot_type_decode(dmr_packet_t *packet)
{
    dmr_log_trace("packet: slot type decode");
    if (packet == NULL)
        return dmr_error(DMR_EINVAL);

    uint8_t bytes[3];
    memset(bytes, 0, sizeof(bytes));
    bytes[0]  = (packet->payload[12] << 2) & B11111100;
    bytes[0] |= (packet->payload[13] >> 6) & B00000011;
    bytes[1]  = (packet->payload[13] << 2) & B11000000;
    bytes[1] |= (packet->payload[19] << 2) & B11110000;
    bytes[1] |= (packet->payload[20] >> 6) & B00000011;
    bytes[2]  = (packet->payload[20] << 2) & B11110000;

    uint8_t code = dmr_golay_20_8_decode(bytes);
    packet->color_code = (code & B11110000) >> 4;
    packet->data_type  = (code & B00001111);

    dmr_log_debug("packet: data type decoded as %s (%d), color code %d",
        dmr_data_type_name(packet->data_type), packet->data_type,
        packet->color_code);

    return 0;
}

int dmr_slot_type_encode(dmr_packet_t *packet)
{
    dmr_log_trace("packet: slot type encode");
    if (packet == NULL)
        return dmr_error(DMR_EINVAL);

    if (packet->data_type >= DMR_DATA_TYPE_INVALID) {
        dmr_log_debug("packet: can't encode slot type of invalid data type 0x%02x",
            packet->data_type);
        return dmr_error(DMR_EINVAL);
    }
    if (packet->color_code < 1 || packet->color_code > 15) {
        dmr_log_debug("packet: can't encode slot type of invalid color code %d",
            packet->color_code);
        return dmr_error(DMR_EINVAL);
    }

    uint8_t bytes[3];
    memset(bytes, 0, sizeof(bytes));
    bytes[0]  = (packet->color_code << 4) & B11110000;
    bytes[0] |= (packet->data_type  << 0) & B00001111;

    dmr_golay_20_8_encode(bytes);

    packet->payload[12] |=                   B11000000;
    packet->payload[12] |= (bytes[0] >> 2) & B00111111;
    packet->payload[13] |=                   B00001111;
    packet->payload[13] |= (bytes[0] << 6) & B11000000;
    packet->payload[13] |= (bytes[1] >> 2) & B00110000;

    packet->payload[19] |=                   B11110000;
    packet->payload[19] |= (bytes[1] >> 2) & B00001111;
    packet->payload[20] |=                   B00000011;
    packet->payload[20] |= (bytes[1] << 6) & B11000000;
    packet->payload[20] |= (bytes[2] >> 2) & B00111100;

    return 0;
}
