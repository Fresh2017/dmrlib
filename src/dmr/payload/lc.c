#include <string.h>
#include <talloc.h>
#include "dmr/error.h"
#include "dmr/malloc.h"
#include "dmr/payload/lc.h"
#include "dmr/packet.h"
#include "dmr/fec/rs_12_9.h"
#include "dmr/fec/bptc_196_96.h"

uint8_t dmr_crc_mask_lc[] = {
    0x69, /* DMR_DATA_TYPE_VOICE_PI */
    0x96, /* DMR_DATA_TYPE_VOICE_LC */
    0x99, /* DMR_DATA_TYPE_TERMINATOR_WITH_LC */
    0xa5, /* DMR_DATA_TYPE_CSBK */
    0xaa, /* DMR_DATA_TYPE_MBC */
    0x00, /* DMR_DATA_TYPE_MBCC (not required) */
    0xcc, /* DMR_DATA_TYPE_DATA */
    0xf0, /* DMR_DATA_TYPE_RATE12_DATA */
    0xff, /* DMR_DATA_TYPE_RATE34_DATA */
    0x00  /* DMR_DATA_TYPE_IDLE (not required) */
};

// Parse a packed Link Control message and checks/corrects the Reed-Solomon check data.
int dmr_full_lc_decode(dmr_full_lc_t *lc, dmr_packet_t *packet)
{
    if (packet == NULL || lc == NULL || packet->data_type >= DMR_DATA_TYPE_INVALID)
        return dmr_error(DMR_EINVAL);

    uint8_t bytes[12];
    memset(bytes, 0, sizeof(bytes));

    // BPTC(196, 96) decode data
    dmr_bptc_196_96_t *bptc = talloc_zero(NULL, dmr_bptc_196_96_t);
    if (bptc == NULL) {
        dmr_log_error("lc: BPTC(196,96) init failed: out of memory");
        return dmr_error(DMR_ENOMEM);
    }

    dmr_log_trace("lc: decoding BPTC(196, 96)");
    if (dmr_bptc_196_96_decode(bptc, packet, bytes) != 0) {
        dmr_log_error("lc: BPTC(196,96) decode failed: %s", dmr_error_get());
        return dmr_error(DMR_LASTERROR);
    }

    dmr_log_trace("LC: apply CRC mask %#02x for data type %s",
        dmr_crc_mask_lc[packet->data_type],
        dmr_data_type_name(packet->data_type));
    bytes[9]  ^= dmr_crc_mask_lc[packet->data_type];
    bytes[10] ^= dmr_crc_mask_lc[packet->data_type];
    bytes[11] ^= dmr_crc_mask_lc[packet->data_type];

    dmr_log_trace("lc: performing Reed-Solomon(12, 9, 4) check on data");
    if (dmr_rs_12_9_4_decode(bytes) != 0) {
        dmr_log_error("LC: parity check failed");
#if defined(DMR_DEBUG_LC)
        dmr_log_debug("LC: parities received:");
        dmr_dump_hex(bytes, 12);
        memset(bytes + 9, 0, 3);
        dmr_rs_12_9_4_encode(bytes);
        dmr_log_debug("LC: parities calculated locally:");
        dmr_dump_hex(bytes, 12);
        dmr_log_trace("LC: apply CRC mask %#02x", dmr_crc_mask_lc[packet->data_type]);
        bytes[9]  ^= dmr_crc_mask_lc[packet->data_type];
        bytes[10] ^= dmr_crc_mask_lc[packet->data_type];
        bytes[11] ^= dmr_crc_mask_lc[packet->data_type];
        dmr_dump_hex(bytes, 12);
#endif
        return -1;
    }

    TALLOC_FREE(bptc);

    lc->flco_pdu = (bytes[0] & 0x3f);
    lc->pf       = 0; // (bytes[0] & 0x80) == 0x80;
    lc->fid      = (bytes[1]);
    lc->dst_id   = (bytes[3] << 16) | (bytes[4] << 8) | (bytes[5]);
    lc->src_id   = (bytes[6] << 16) | (bytes[7] << 8) | (bytes[8]);
    memcpy(lc->crc, bytes + 9, 3);

    if (dmr_log_priority() <= DMR_LOG_PRIORITY_DEBUG) {
        dmr_log_debug("LC: bytes (with parity):");
        dmr_dump_hex(bytes, 12);
    }

    return 0;
}

int dmr_full_lc_encode_bytes(dmr_full_lc_t *lc, uint8_t bytes[12])
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
        dmr_dump_hex(bytes, 12);
    }

    // Calculate RS(12, 9) checksum
    dmr_log_trace("LC: calculating Reed-Solomon (12, 9, 4) parities:");
    dmr_rs_12_9_4_encode(bytes);
    if (dmr_log_priority() <= DMR_LOG_PRIORITY_DEBUG) {
        dmr_dump_hex(bytes, 12);
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
    memset(bytes, 0, sizeof(bytes));
    if (dmr_full_lc_encode_bytes(lc, bytes) != 0)
        return dmr_error(DMR_LASTERROR);

    dmr_log_trace("LC: apply CRC mask %#02x", dmr_crc_mask_lc[packet->data_type]);
    bytes[9]  ^= dmr_crc_mask_lc[packet->data_type];
    bytes[10] ^= dmr_crc_mask_lc[packet->data_type];
    bytes[11] ^= dmr_crc_mask_lc[packet->data_type];

    // BPTC(196, 96) encode data
    dmr_bptc_196_96_t bptc;
    dmr_log_trace("lc: encoding BPTC(196, 96)");
    if (dmr_bptc_196_96_encode(&bptc, packet, bytes) != 0)
        return dmr_error(DMR_LASTERROR);

    if (dmr_log_priority() <= DMR_LOG_PRIORITY_DEBUG) {
        dmr_dump_hex(packet->payload, DMR_PAYLOAD_BYTES);
    }

    return 0;
}

char *dmr_flco_pdu_name(dmr_flco_pdu_t flco_pdu)
{
    switch (flco_pdu) {
    case DMR_FLCO_PDU_GROUP:
        return "group";
    case DMR_FLCO_PDU_PRIVATE:
        return "unit to unit";
    default:
        return "invalid";
    }
}
