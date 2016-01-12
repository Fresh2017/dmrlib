#include <string.h>
#include "dmr/error.h"
#include "dmr/payload/lc.h"
#include "dmr/packet.h"
#include "dmr/fec/rs_12_9.h"
#include "dmr/fec/bptc_196_96.h"

static const uint8_t DMR_CRC_MASK_VOICE_LC[]           = {0x96, 0x96, 0x96};
static const uint8_t DMR_CRC_MASK_TERMINATOR_WITH_LC[] = {0x99, 0x99, 0x99};

// Parse a packed Link Control message and checks/corrects the Reed-Solomon check data.
int dmr_lc_full_decode(dmr_lc_t *lc, dmr_packet_t *packet)
{
    if (packet == NULL || lc == NULL)
        return dmr_error(DMR_EINVAL);

    uint8_t buf[DMR_FEC_RS_12_9_DATASIZE + DMR_FEC_RS_12_9_CHECKSUMSIZE];
    memset(buf, 0, sizeof(buf));

    // BPTC(196, 96) decode data
    dmr_bptc_196_96_t bptc;
    dmr_log_trace("lc: decoding BPTC(196, 96)");
    if (dmr_bptc_196_96_decode(&bptc, packet, buf) != 0)
        return dmr_error(DMR_LASTERROR);

    // Retrieve checksum with CRC mask applied. See DMR AI. spec. page 143.
    uint8_t i = 0;
    for (; i < DMR_FEC_RS_12_9_CHECKSUMSIZE; i++)
        buf[DMR_FEC_RS_12_9_DATASIZE + i] ^= 0x96;

    //dmr_log_trace("lc: verifying RS(12, 9) checksum:");
    lc->flco = buf[0];
    lc->fid = buf[1];
    lc->service_options.byte = buf[2];
    lc->dst_id = (buf[3] << 16) | (buf[4] << 8) | buf[5];
    lc->src_id = (buf[6] << 16) | (buf[7] << 8) | buf[8];

    return 0;
}

int dmr_lc_full_encode(dmr_lc_t *lc, dmr_packet_t *packet)
{
    if (packet == NULL || lc == NULL)
        return dmr_error(DMR_EINVAL);

    if (packet->slot_type != DMR_SLOT_TYPE_VOICE_LC &&
        packet->slot_type != DMR_SLOT_TYPE_TERMINATOR_WITH_LC) {
        dmr_log_error("lc: unsupported LC type %s",
            dmr_slot_type_name(packet->slot_type));
        return -1;
    }

    // Construct full LC
    dmr_log_trace("lc: constructing full lc, flco %d, fid %d, so %d, %lu->%lu:",
        lc->flco, lc->fid, lc->service_options.byte, lc->src_id, lc->dst_id);
    uint8_t buf[DMR_FEC_RS_12_9_DATASIZE + DMR_FEC_RS_12_9_CHECKSUMSIZE] = {
        lc->flco,
        lc->fid,
        lc->service_options.byte,
        lc->dst_id >> 16,
        lc->dst_id >> 8,
        lc->dst_id,
        lc->src_id >> 16,
        lc->src_id >> 8,
        lc->src_id,
        0, 0, 0 /* 3 bits RS(12, 9) checksum */
    };

    if (dmr_log_priority() <= DMR_LOG_PRIORITY_DEBUG) {
        dump_hex(buf, DMR_FEC_RS_12_9_DATASIZE);
    }

    // Calculate RS(12, 9) checksum
    dmr_log_trace("lc: calculating RS(12, 9) checksum:");
    dmr_fec_rs_12_9_codeword_t codeword;
    memcpy(&codeword, buf, sizeof(dmr_fec_rs_12_9_codeword_t));
    dmr_fec_rs_12_9_checksum_t *parity = dmr_fec_rs_12_9_calc_checksum(&codeword);

    if (dmr_log_priority() <= DMR_LOG_PRIORITY_DEBUG) {
        dump_hex(buf + DMR_FEC_RS_12_9_DATASIZE, DMR_FEC_RS_12_9_CHECKSUMSIZE);
    }

    // Store checksum with CRC mask applied. See DMR AI. spec. page 143.
    switch (packet->slot_type) {
    case DMR_SLOT_TYPE_VOICE_LC:
        buf[0x09] = parity->bytes[0] ^ DMR_CRC_MASK_VOICE_LC[0];
        buf[0x0a] = parity->bytes[1] ^ DMR_CRC_MASK_VOICE_LC[1];
        buf[0x0b] = parity->bytes[2] ^ DMR_CRC_MASK_VOICE_LC[2];
        break;
    case DMR_SLOT_TYPE_TERMINATOR_WITH_LC:
        buf[0x09] = parity->bytes[0] ^ DMR_CRC_MASK_TERMINATOR_WITH_LC[0];
        buf[0x0a] = parity->bytes[1] ^ DMR_CRC_MASK_TERMINATOR_WITH_LC[1];
        buf[0x0b] = parity->bytes[2] ^ DMR_CRC_MASK_TERMINATOR_WITH_LC[2];
        break;
    default:
        break;
    }

    // BPTC(196, 96) encode data
    dmr_bptc_196_96_t bptc;
    dmr_log_trace("lc: encoding BPTC(196, 96)");
    if (dmr_bptc_196_96_encode(&bptc, packet, buf) != 0)
        return dmr_error(DMR_LASTERROR);

    if (dmr_log_priority() <= DMR_LOG_PRIORITY_DEBUG) {
        dump_hex(packet->payload, DMR_PAYLOAD_BYTES);
    }

    return 0;
}
