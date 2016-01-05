#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "dmr/proto/homebrew.h"
#include "dmr/type.h"

const char dmr_homebrew_data_signature[4] = {'D', 'M', 'R', 'D'};

dmr_homebrew_frame_type_t dmr_homebrew_frame_type(const uint8_t *bytes, unsigned int len)
{
    switch (len) {
    case DMR_HOMEBREW_DMR_DATA_SIZE:
        if (!memcmp(bytes, dmr_homebrew_data_signature, 4))
            return DMR_HOMEBREW_DMR_DATA_FRAME;

        break;
    default:
        break;
    }

    return DMR_HOMEBREW_UNKNOWN_FRAME;
}

/* The result from this function needs to be freed */
dmr_homebrew_packet_t *dmr_homebrew_parse_packet(const uint8_t *bytes, unsigned int len)
{
    if (dmr_homebrew_frame_type(bytes, len) != DMR_HOMEBREW_DMR_DATA_FRAME)
        return NULL;

    dmr_homebrew_packet_t *packet = malloc(sizeof(dmr_homebrew_packet_t));
    if (bytes == NULL)
        return NULL;

    memset(packet, 0, sizeof(dmr_homebrew_packet_t));
    memcpy(packet->signature, bytes, 4);
    packet->sequence = bytes[4];
    packet->dmr_packet.src_id = (bytes[5] << 16) | (bytes[6] << 8) | bytes[7];
    packet->dmr_packet.dst_id = (bytes[8] << 16) | (bytes[9] << 8) | bytes[10];
    packet->dmr_packet.repeater_id = bytes[11] | (bytes[12] << 8) | (bytes[13] << 16) | (bytes[14] << 24);
    packet->dmr_packet.ts = (bytes[15]) & 0x01;
    packet->call_type = (bytes[15] >> 1) & 0x01;
    packet->frame_type = (bytes[15] >> 2) & 0x03;
    packet->data_type = (bytes[15] >> 4) & 0x0f;

    switch (packet->frame_type) {
    case DMR_HOMEBREW_DATA_TYPE_VOICE:
    case DMR_HOMEBREW_DATA_TYPE_VOICE_SYNC:
        packet->dmr_packet.slot_type = DMR_SLOT_TYPE_VOICE_BURST_A + packet->data_type;
		break;
	case DMR_HOMEBREW_DATA_TYPE_DATA_SYNC: // data sync
		packet->dmr_packet.slot_type = (dmr_slot_type_t) packet->data_type;
		break;
    default:
        packet->dmr_packet.slot_type = DMR_SLOT_TYPE_UNKNOWN;
        break;
    }

    memcpy(&packet->dmr_packet.payload.bytes, &bytes[20], DMR_PAYLOAD_BYTES);
    bytes_to_bits(&packet->dmr_packet.payload.bytes[0], DMR_PAYLOAD_BYTES,
                  &packet->dmr_packet.payload.bits[0], DMR_PAYLOAD_BITS);

    return packet;
}
