#include <string.h>
#include "dmr/error.h"
#include "dmr/payload/lc.h"
#include "dmr/packet.h"
#include "dmr/fec/rs_12_9.h"
#include "dmr/fec/bptc_196_96.h"

static const uint8_t DMR_CRC_MASK_VOICE_LC[]           = {0x96, 0x96, 0x96};
static const uint8_t DMR_CRC_MASK_TERMINATOR_WITH_LC[] = {0x99, 0x99, 0x99};

// Parse a packed Link Control message and checks/corrects the Reed-Solomon check data.
int dmr_full_lc_decode(dmr_full_lc_t *lc, dmr_packet_t *packet)
{
    if (packet == NULL || lc == NULL)
        return dmr_error(DMR_EINVAL);

    uint8_t bytes[12];
    memset(bytes, 0, sizeof(bytes));

    // BPTC(196, 96) decode data
    dmr_bptc_196_96_t bptc;
    dmr_log_trace("lc: decoding BPTC(196, 96)");
    if (dmr_bptc_196_96_decode(&bptc, packet, bytes) != 0)
        return dmr_error(DMR_LASTERROR);

    lc->flco_pdu = (bytes[0] & 0x3f);
    lc->privacy  = (bytes[0] & 0x80) == 0x80;
    lc->fid      = (bytes[1]);
    lc->dst_id   = (bytes[3] << 16) | (bytes[4] << 8) | (bytes[5]);
    lc->src_id   = (bytes[6] << 16) | (bytes[7] << 8) | (bytes[8]);
    memcpy(lc->crc, bytes + DMR_RS_12_9_DATASIZE, sizeof(lc->crc));

    // Retrieve checksum with CRC mask applied. See DMR AI. spec. page 143.
    switch (packet->data_type) {
    case DMR_DATA_TYPE_VOICE_LC:
        lc->crc[0] ^= DMR_CRC_MASK_VOICE_LC[0];
        lc->crc[1] ^= DMR_CRC_MASK_VOICE_LC[1];
        lc->crc[2] ^= DMR_CRC_MASK_VOICE_LC[2];
        break;
    case DMR_DATA_TYPE_TERMINATOR_WITH_LC:
        lc->crc[0] ^= DMR_CRC_MASK_TERMINATOR_WITH_LC[0];
        lc->crc[1] ^= DMR_CRC_MASK_TERMINATOR_WITH_LC[1];
        lc->crc[2] ^= DMR_CRC_MASK_TERMINATOR_WITH_LC[2];
        break;
    default:
        dmr_log_error("lc: unsupported data type %s", dmr_data_type_name(packet->data_type));
        return -1;
    }

    return 0;
}

int dmr_full_lc_encode_bytes(dmr_full_lc_t *lc, uint8_t bytes[12], dmr_data_type_t data_type)
{
    if (lc == NULL || bytes == NULL)
        return dmr_error(DMR_EINVAL);

    memset(bytes, 0, 12);
    bytes[0]  = lc->flco_pdu;
    bytes[0] |= (lc->flco_pdu == DMR_FLCO_PDU_PRIVATE) ? 0x80 : 0x00;
    bytes[1]  = lc->fid;
    bytes[2]  = 0;
    bytes[3]  = lc->dst_id >> 16;
    bytes[4]  = lc->dst_id >>  8;
    bytes[5]  = lc->dst_id >>  0;
    bytes[6]  = lc->src_id >> 16;
    bytes[7]  = lc->src_id >>  8;
    bytes[8]  = lc->src_id >>  0;

    if (dmr_log_priority() <= DMR_LOG_PRIORITY_DEBUG) {
        dump_hex(bytes, DMR_RS_12_9_DATASIZE);
    }

    // Calculate RS(12, 9) checksum
    dmr_log_trace("lc: calculating RS(12, 9) checksum:");
    dmr_rs_12_9_codeword_t codeword;
    dmr_rs_12_9_checksum_t *parity = dmr_rs_12_9_calc_checksum((dmr_rs_12_9_codeword_t *)bytes);

    if (dmr_log_priority() <= DMR_LOG_PRIORITY_DEBUG) {
        dump_hex(bytes + DMR_RS_12_9_DATASIZE, DMR_RS_12_9_CHECKSUMSIZE);
    }

    // Store checksum with CRC mask applied. See DMR AI. spec. page 143.
    switch (data_type) {
    case DMR_DATA_TYPE_VOICE_LC:
        bytes[9]  = parity->bytes[2] ^ DMR_CRC_MASK_VOICE_LC[0];
        bytes[10] = parity->bytes[1] ^ DMR_CRC_MASK_VOICE_LC[1];
        bytes[11] = parity->bytes[0] ^ DMR_CRC_MASK_VOICE_LC[2];
        break;
    case DMR_DATA_TYPE_TERMINATOR_WITH_LC:
        bytes[9]  = parity->bytes[2] ^ DMR_CRC_MASK_TERMINATOR_WITH_LC[0];
        bytes[10] = parity->bytes[1] ^ DMR_CRC_MASK_TERMINATOR_WITH_LC[1];
        bytes[11] = parity->bytes[0] ^ DMR_CRC_MASK_TERMINATOR_WITH_LC[2];
        break;
    default:
        break;
    }

    return 0;
}

int dmr_full_lc_encode(dmr_full_lc_t *lc, dmr_packet_t *packet)
{
    if (packet == NULL || lc == NULL)
        return dmr_error(DMR_EINVAL);

    if (packet->data_type != DMR_DATA_TYPE_VOICE_LC &&
        packet->data_type != DMR_DATA_TYPE_TERMINATOR_WITH_LC) {
        dmr_log_error("lc: unsupported LC type %s",
            dmr_data_type_name(packet->data_type));
        return -1;
    }

    // Construct full LC
    dmr_log_trace("lc: constructing full lc, flco %d, fid %d, so %d, %u->%u:",
        lc->flco_pdu, lc->fid, lc->service_options,
        lc->src_id, lc->dst_id);

    uint8_t bytes[12];
    if (dmr_full_lc_encode_bytes(lc, bytes, packet->data_type) != 0)
        return dmr_error(DMR_LASTERROR);

    // BPTC(196, 96) encode data
    dmr_bptc_196_96_t bptc;
    dmr_log_trace("lc: encoding BPTC(196, 96)");
    if (dmr_bptc_196_96_encode(&bptc, packet, bytes) != 0)
        return dmr_error(DMR_LASTERROR);

    if (dmr_log_priority() <= DMR_LOG_PRIORITY_DEBUG) {
        dump_hex(packet->payload, DMR_PAYLOAD_BYTES);
    }

    return 0;
}
