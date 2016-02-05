#include <string.h>
#ifdef DMR_DEBUG
#include <assert.h>
#endif
#include <talloc.h>
#include "dmr/error.h"
#include "dmr/payload/emb.h"
#include "dmr/fec/qr_16_7.h"
#include "dmr/fec/vbptc_16_11.h"

int dmr_emb_decode(dmr_packet packet, dmr_emb *emb)
{
    if (emb == NULL || packet == NULL)
        return dmr_error(DMR_EINVAL);

    uint8_t emb_bytes[2];
    memset(emb_bytes, 0, sizeof(emb_bytes));
    emb_bytes[0]  = (packet[13] << 4) & 0xf0;
    emb_bytes[0] |= (packet[14] >> 4) & 0x0f;
    emb_bytes[1]  = (packet[18] << 4) & 0xf0;
    emb_bytes[1] |= (packet[19] >> 4) & 0x0f;
    dmr_log_trace("emb: bytes: %02x %02x", emb_bytes[0], emb_bytes[1]);
    if (!dmr_qr_16_7_decode(emb_bytes)) {
        dmr_log_debug("emb: qr_16_7 checksum failed");
        return -1;
    }

    /* See Table E.6: Transmit bit order for voice burst with embedded signalling fragment 1 */
    emb->color_code = (emb_bytes[0] >> 4) & 0x0f;
    emb->pi         = (emb_bytes[0] & 0x08) == 0x08;
    emb->lcss       = (emb_bytes[0] >> 5) & 0x07;

    return 0;
}

int dmr_emb_bytes_decode(dmr_packet packet, uint8_t bytes[4])
{
    if (bytes == NULL || packet == NULL)
        return dmr_error(DMR_EINVAL);

    bytes[0]  = (packet[14] << 4) & 0xf0;
    bytes[0] |= (packet[15] >> 4) & 0x0f;
    bytes[1]  = (packet[15] << 4) & 0xf0;
    bytes[1] |= (packet[16] >> 4) & 0x0f;
    return 0;
}

bool dmr_emb_null(uint8_t bytes[4])
{
    uint8_t i;
    for (i = 0; i < 4; i++) {
        if (bytes[i] != 0x00)
            return false;
    }
    return true;
}

int dmr_emb_encode(dmr_packet packet, dmr_emb *emb)
{
    if (emb == NULL || packet == NULL)
        return dmr_error(DMR_EINVAL);

    uint8_t emb_bytes[2];
    emb_bytes[0]  = (emb->color_code & 0x0f);
    emb_bytes[0] |= (emb->pi  ? 0x01 : 0x00) << 4;
    emb_bytes[0] |= (emb->lcss       & 0x03) << 5;
    emb_bytes[1]  = 0; // Will be calculated
    dmr_qr_16_7_encode(emb_bytes);

    packet[13] = (packet[13] & 0xf0U) | (emb_bytes[0] >> 4);
    packet[14] = (packet[14] & 0x0fU) | (emb_bytes[0] << 4);
    packet[18] = (packet[18] & 0xf0U) | (emb_bytes[1] >> 4);
    packet[19] = (packet[19] & 0x0fU) | (emb_bytes[1] << 4);
    return 0;
}

dmr_emb_signalling_lc_bits *dmr_emb_signalling_lc_interlave(dmr_emb_signalling_lc_bits *emb_bits)
{
    if (emb_bits == NULL)
        return NULL;

    dmr_emb_signalling_lc_bits *interleaved = talloc(emb_bits, dmr_emb_signalling_lc_bits);
    if (interleaved == NULL)
        return NULL;

    bool *bits = (bool *)interleaved;
    uint8_t i, j;

    for (i = 0, j = 0; i < sizeof(dmr_emb_signalling_lc_bits); i++) {
        switch (i) {
        case 32:
            bits[i] = emb_bits->checksum[0];
            break;
        case 43:
            bits[i] = emb_bits->checksum[1];
            break;
        case 54:
            bits[i] = emb_bits->checksum[2];
            break;
        case 65:
            bits[i] = emb_bits->checksum[3];
            break;
        case 76:
            bits[i] = emb_bits->checksum[4];
            break;
        default:
            bits[i] = emb_bits->bits[j++];
            break;
        }
    }

    return interleaved;
}

static int dmr_emb_encode_signalling_lc_bytes(uint8_t *bytes, dmr_emb_signalling_lc_bits *emb_bits)
{
    if (bytes == NULL || emb_bits == NULL)
        return dmr_error(DMR_EINVAL);

    uint8_t i;
    uint16_t checksum = 0;
    for (i = 0; i < 9; i++) {
        checksum += (uint16_t)bytes[i];
    }
    checksum %= 31;
    for (i = 0; i < 5; i++)
        emb_bits->checksum[i] = (checksum >> (4 - i)) > 0;

    dmr_bytes_to_bits(bytes, sizeof(bytes), emb_bits->bits, sizeof(bytes) * 8);
    return 0;
}

int dmr_emb_encode_signalling_lc_from_full_lc(dmr_full_lc *lc, dmr_emb_signalling_lc_bits *emb_bits, dmr_data_type data_type)
{
    if (emb_bits == NULL || lc == NULL || data_type >= DMR_DATA_TYPE_INVALID)
        return dmr_error(DMR_EINVAL);

    uint8_t bytes[12];
    if (dmr_full_lc_encode_bytes(lc, bytes) != 0)
        return dmr_error(DMR_LASTERROR);

    return dmr_emb_encode_signalling_lc_bytes(bytes, emb_bits);
}

int dmr_emb_encode_signalling_lc(dmr_packet packet, dmr_emb_signalling_lc_bits *emb_bits, dmr_lc *lc)
{
    if (emb_bits == NULL || packet == NULL || lc == NULL)
        return dmr_error(DMR_EINVAL);

    uint8_t bytes[9];
    memset(bytes, 0, sizeof(bytes));
    if (lc->flco == DMR_FLCO_PRIVATE) {
        bytes[0] = 3;
    }
    bytes[3] = lc->dst_id >> 16;
    bytes[4] = lc->dst_id >> 8;
    bytes[5] = lc->dst_id;
    bytes[6] = lc->src_id >> 16;
    bytes[7] = lc->src_id >> 8;
    bytes[8] = lc->src_id;

    return dmr_emb_encode_signalling_lc_bytes(bytes, emb_bits);
}

char *dmr_emb_lcss_name(dmr_emb_lcss lcss)
{
    switch (lcss) {
    case DMR_EMB_LCSS_SINGLE_FRAGMENT:
        return "single fragment";
    case DMR_EMB_LCSS_FIRST_FRAGMENT:
        return "first fragment";
    case DMR_EMB_LCSS_LAST_FRAGMENT:
        return "last fragment";
    case DMR_EMB_LCSS_CONTINUATION:
        return "continuation";
    default:
        return "invalid";
    }
}

int dmr_emb_lcss_fragment_encode(dmr_packet packet, dmr_emb *emb, dmr_vbptc_16_11 *vbptc, uint8_t fragment)
{
    if (emb == NULL || packet == NULL)
        return dmr_error(DMR_EINVAL);

    bool bits[32];
    uint8_t lc_bytes[4];
    memset(lc_bytes, 0, sizeof(lc_bytes));
    uint16_t offset = (uint16_t)(fragment) * 32;
    memset(bits, 0, sizeof(bits));

    if (vbptc != NULL && dmr_vbptc_16_11_get_fragment(vbptc, bits, offset, 32) != 0) {
       return dmr_error(DMR_LASTERROR);
    }
    dmr_bits_to_bytes(bits, sizeof(bits), lc_bytes, sizeof(lc_bytes));

    packet[14] = (packet[14] & 0xf0) | (lc_bytes[0] >> 4);
    packet[15] = (lc_bytes[0]  << 4) | (lc_bytes[1] >> 4);
    packet[16] = (lc_bytes[1]  << 4) | (lc_bytes[2] >> 4);
    packet[17] = (lc_bytes[2]  << 4) | (lc_bytes[3] >> 4);
    packet[18] = (lc_bytes[3]  << 4) | (packet[18]  >> 4);

    return dmr_emb_encode(packet, emb);
}
