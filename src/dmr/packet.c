#include <stdlib.h>
#include <string.h>
#include "dmr/error.h"
#include "dmr/log.h"
#include "dmr/packet.h"

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

    memcpy(packet->payload.bytes, buf, DMR_PAYLOAD_BYTES);
    bytes_to_bits(packet->payload.bytes, DMR_PAYLOAD_BYTES,
                  packet->payload.bits, DMR_PAYLOAD_BITS);

    return packet;
}

char *dmr_packet_get_slot_type_name(dmr_slot_type_t slot_type)
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
