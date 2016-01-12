#include <stdlib.h>
#include <string.h>
#include "dmr/bits.h"
#include "dmr/error.h"
#include "dmr/log.h"
#include "dmr/packet.h"
#include "dmr/fec/golay_20_8.h"

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

char *dmr_slot_type_name(dmr_slot_type_t slot_type)
{
    switch (slot_type) {
    case DMR_SLOT_TYPE_PRIVACY_INDICATOR:
        return "privacy indicator";
    case DMR_SLOT_TYPE_VOICE_LC:
        return "voice lc";
    case DMR_SLOT_TYPE_TERMINATOR_WITH_LC:
        return "terminator with lc";
    case DMR_SLOT_TYPE_CSBK:
        return "csbk";
    case DMR_SLOT_TYPE_MULTI_BLOCK_CONTROL:
        return "multi block control";
    case DMR_SLOT_TYPE_MULTI_BLOCK_CONTROL_CONT:
        return "multi block control continuation";
    case DMR_SLOT_TYPE_DATA:
        return "data";
    case DMR_SLOT_TYPE_RATE12_DATA:
        return "rate 1/2 data";
    case DMR_SLOT_TYPE_RATE34_DATA:
        return "rate 3/4 data";
    case DMR_SLOT_TYPE_IDLE:
        return "idle";
    case DMR_SLOT_TYPE_VOICE_BURST_A:
        return "voice burst A";
    case DMR_SLOT_TYPE_VOICE_BURST_B:
        return "voice burst B";
    case DMR_SLOT_TYPE_VOICE_BURST_C:
        return "voice burst C";
    case DMR_SLOT_TYPE_VOICE_BURST_D:
        return "voice burst D";
    case DMR_SLOT_TYPE_VOICE_BURST_E:
        return "voice burst E";
    case DMR_SLOT_TYPE_VOICE_BURST_F:
        return "voice burst F";
    case DMR_SLOT_TYPE_SYNC_VOICE:
        return "sync voice";
    case DMR_SLOT_TYPE_SYNC_DATA:
        return "sync data";
    case DMR_SLOT_TYPE_UNKNOWN:
    default:
        return "unknown";
    }
}

int dmr_slot_type_decode(dmr_slot_t *slot, dmr_packet_t *packet)
{
    dmr_log_trace("packet: slot type decode");
    if (slot == NULL || packet == NULL)
        return dmr_error(DMR_EINVAL);

    slot->bytes[0]  = (packet->payload[12] << 2) & B11111100;
    slot->bytes[0] |= (packet->payload[13] >> 6) & B00000011;
    slot->bytes[1]  = (packet->payload[13] << 2) & B11000000;
    slot->bytes[1] |= (packet->payload[19] << 2) & B11110000;
    slot->bytes[1] |= (packet->payload[20] >> 6) & B00000011;
    slot->bytes[2]  = (packet->payload[20] << 2) & B11110000;

    packet->slot_type = slot->fields.data_type;
    dmr_log_debug("packet: slot type decoded as %s (%d)",
        dmr_slot_type_name(packet->slot_type), packet->slot_type);

    return 0;
}

int dmr_slot_type_encode(dmr_slot_t *slot, dmr_packet_t *packet)
{
    dmr_log_trace("packet: slot type encode");
    if (slot == NULL || packet == NULL)
        return dmr_error(DMR_EINVAL);
    if (packet->slot_type > 0x0fU) {
        dmr_log_error("packet: can't set slot type PDU for slot type %s (0x%02x)",
            dmr_slot_type_name(packet->slot_type), packet->slot_type);
        return -1;
    }

    slot->fields.fec = dmr_golay_20_8_encode(slot->bytes[0]);

    packet->payload[12] |=                         B11000000;
    packet->payload[12] |= (slot->bytes[0] >> 2) & B00111111;
    packet->payload[13] |=                         B00001111;
    packet->payload[13] |= (slot->bytes[0] << 6) & B11000000;
    packet->payload[13] |= (slot->bytes[1] >> 2) & B00110000;

    packet->payload[19] |=                         B11110000;
    packet->payload[19] |= (slot->bytes[1] >> 2) & B00001111;
    packet->payload[20] |=                         B00000011;
    packet->payload[20] |= (slot->bytes[1] << 6) & B11000000;
    packet->payload[20] |= (slot->bytes[2] >> 2) & B00111100;

    return 0;
}
