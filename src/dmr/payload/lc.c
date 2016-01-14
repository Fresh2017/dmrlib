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

    memset(lc->bytes, 0, sizeof(lc->bytes));

    // BPTC(196, 96) decode data
    dmr_bptc_196_96_t bptc;
    dmr_log_trace("lc: decoding BPTC(196, 96)");
    if (dmr_bptc_196_96_decode(&bptc, packet, lc->bytes) != 0)
        return dmr_error(DMR_LASTERROR);

    // Retrieve checksum with CRC mask applied. See DMR AI. spec. page 143.
    switch (packet->data_type) {
    case DMR_DATA_TYPE_VOICE_LC:
        lc->fields.crc[0] ^= DMR_CRC_MASK_VOICE_LC[0];
        lc->fields.crc[1] ^= DMR_CRC_MASK_VOICE_LC[1];
        lc->fields.crc[2] ^= DMR_CRC_MASK_VOICE_LC[2];
        break;
    case DMR_DATA_TYPE_TERMINATOR_WITH_LC:
        lc->fields.crc[0] ^= DMR_CRC_MASK_TERMINATOR_WITH_LC[0];
        lc->fields.crc[1] ^= DMR_CRC_MASK_TERMINATOR_WITH_LC[1];
        lc->fields.crc[2] ^= DMR_CRC_MASK_TERMINATOR_WITH_LC[2];
        break;
    default:
        dmr_log_error("lc: unsupported data type %s", dmr_data_type_name(packet->data_type));
        return -1;
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
    dmr_log_trace("lc: constructing full lc, flco %d, fid %d, so %d, %lu->%lu:",
        lc->fields.flco, lc->fields.fid, lc->fields.service_options,
        DMR_UINT32_BE_UNPACK3(lc->fields.src_id),
        DMR_UINT32_BE_UNPACK3(lc->fields.dst_id));

    if (dmr_log_priority() <= DMR_LOG_PRIORITY_DEBUG) {
        dump_hex(lc->bytes, DMR_RS_12_9_DATASIZE);
    }

    // Calculate RS(12, 9) checksum
    dmr_log_trace("lc: calculating RS(12, 9) checksum:");
    dmr_rs_12_9_codeword_t codeword;
    dmr_rs_12_9_checksum_t *parity = dmr_rs_12_9_calc_checksum((dmr_rs_12_9_codeword_t *)lc->bytes);

    if (dmr_log_priority() <= DMR_LOG_PRIORITY_DEBUG) {
        dump_hex(lc->fields.crc, DMR_RS_12_9_CHECKSUMSIZE);
    }

    // Store checksum with CRC mask applied. See DMR AI. spec. page 143.
    switch (packet->data_type) {
    case DMR_DATA_TYPE_VOICE_LC:
        lc->fields.crc[0] = parity->bytes[2] ^ DMR_CRC_MASK_VOICE_LC[0];
        lc->fields.crc[1] = parity->bytes[1] ^ DMR_CRC_MASK_VOICE_LC[1];
        lc->fields.crc[2] = parity->bytes[0] ^ DMR_CRC_MASK_VOICE_LC[2];
        break;
    case DMR_DATA_TYPE_TERMINATOR_WITH_LC:
        lc->fields.crc[0] = parity->bytes[2] ^ DMR_CRC_MASK_TERMINATOR_WITH_LC[0];
        lc->fields.crc[1] = parity->bytes[1] ^ DMR_CRC_MASK_TERMINATOR_WITH_LC[1];
        lc->fields.crc[2] = parity->bytes[0] ^ DMR_CRC_MASK_TERMINATOR_WITH_LC[2];
        break;
    default:
        break;
    }

    // BPTC(196, 96) encode data
    dmr_bptc_196_96_t bptc;
    dmr_log_trace("lc: encoding BPTC(196, 96)");
    if (dmr_bptc_196_96_encode(&bptc, packet, lc->bytes) != 0)
        return dmr_error(DMR_LASTERROR);

    if (dmr_log_priority() <= DMR_LOG_PRIORITY_DEBUG) {
        dump_hex(packet->payload, DMR_PAYLOAD_BYTES);
    }

    return 0;
}
