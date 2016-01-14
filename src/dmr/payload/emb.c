#include <string.h>
#ifdef DMR_DEBUG
#include <assert.h>
#endif
#include <talloc.h>
#include "dmr/error.h"
#include "dmr/payload/emb.h"
#include "dmr/fec/qr_16_7.h"
#include "dmr/fec/vbptc_16_11.h"

int dmr_emb_decode(dmr_emb_t *emb, dmr_packet_t *packet)
{
    if (emb == NULL || packet == NULL)
        return dmr_error(DMR_EINVAL);

#ifdef DMR_DEBUG
    assert(sizeof(dmr_emb_t) == 2);
#endif
    memset(emb, 0, sizeof(dmr_emb_t));
    emb->bytes[0]  = (packet->payload[13] << 4) & 0xf0U;
    emb->bytes[0] |= (packet->payload[14] >> 4) & 0x0fU;
    emb->bytes[1]  = (packet->payload[18] << 4) & 0xf0U;
    emb->bytes[1] |= (packet->payload[19] >> 4) & 0x0fU;
    //dmr_fec_qr_16_7_decode(emb->bytes);
    return 0;
}

int dmr_emb_encode(dmr_emb_t *emb, dmr_packet_t *packet)
{
    if (emb == NULL || packet == NULL)
        return dmr_error(DMR_EINVAL);

    dmr_fec_qr_16_7_encode(emb->bytes);
    packet->payload[13] = (packet->payload[13] & 0xf0U) | (emb->bytes[0] >> 4);
    packet->payload[14] = (packet->payload[14] & 0x0fU) | (emb->bytes[0] << 4);
    packet->payload[18] = (packet->payload[18] & 0xf0U) | (emb->bytes[1] >> 4);
    packet->payload[19] = (packet->payload[19] & 0x0fU) | (emb->bytes[1] << 4);
    return 0;
}

dmr_emb_signalling_lc_bits_t *dmr_emb_signalling_lc_interlave(dmr_emb_signalling_lc_bits_t *emb_bits)
{
    if (emb_bits == NULL)
        return NULL;

    dmr_emb_signalling_lc_bits_t *interleaved = talloc(emb_bits, dmr_emb_signalling_lc_bits_t);
    if (interleaved == NULL)
        return NULL;

    bool *bits = (bool *)interleaved;
    uint8_t i, j;

    for (i = 0, j = 0; i < sizeof(dmr_emb_signalling_lc_bits_t); i++) {
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

static int dmr_emb_encode_signalling_lc_bytes(uint8_t *bytes, dmr_emb_signalling_lc_bits_t *emb_bits)
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

int dmr_emb_encode_signalling_lc_from_full_lc(dmr_full_lc_t *lc, dmr_emb_signalling_lc_bits_t *emb_bits)
{
    if (emb_bits == NULL || lc == NULL)
        return dmr_error(DMR_EINVAL);

    return dmr_emb_encode_signalling_lc_bytes(lc->bytes, emb_bits);
}

int dmr_emb_encode_signalling_lc(dmr_emb_signalling_lc_bits_t *emb_bits, dmr_packet_t *packet)
{
    if (emb_bits == NULL || packet == NULL)
        return dmr_error(DMR_EINVAL);

    uint8_t bytes[9];
    memset(bytes, 0, sizeof(bytes));
    if (packet->flco == DMR_FLCO_PRIVATE) {
        bytes[0] = 3;
    }
    DMR_UINT32_BE_PACK3(bytes + 3, packet->dst_id);
    DMR_UINT32_BE_PACK3(bytes + 6, packet->src_id);

    return dmr_emb_encode_signalling_lc_bytes(bytes, emb_bits);
}

char *dmr_emb_lcss_name(dmr_emb_lcss_t lcss)
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

int dmr_emb_lcss_fragment_encode(dmr_emb_lcss_t lcss, dmr_vbptc_16_11_t *vbptc, uint8_t fragment, dmr_packet_t *packet)
{
    if (vbptc == NULL || packet == NULL)
        return dmr_error(DMR_EINVAL);

    bool bits[32];
    uint8_t bytes[4];
    uint16_t offset = (uint16_t)(fragment) * 32;
    memset(bits, 0, sizeof(bits));
    if (dmr_vbptc_16_11_get_fragment(vbptc, bits, offset, 32) != 0)
        return dmr_error(DMR_LASTERROR);
    dmr_bits_to_bytes(bits, sizeof(bits), bytes, sizeof(bytes));

    dmr_emb_t emb = {
        .flags = {
            .color_code = packet->color_code,
            .pi         = 0,
            .lcss       = lcss,
            .qr0        = 0,
            .qr1        = 0
        }
    };
    dmr_fec_qr_16_7_encode(emb.bytes);

    // Figure 6.4: Voice burst with embedded signalling
    // |  VOICE (108 bits)  |  SYNC (48 bits)  |  VOICE (108 bits)  |
    // |  VOICE (108 bits)  |E|  ES (32 bits)|E|  VOICE (108 bits)  |
    packet->payload[12] = (packet->payload[12] & 0xf0) | (emb.bytes[0] >> 4);
    packet->payload[13] = (emb.bytes[0] << 4)          | (bytes[0]     >> 4);
    packet->payload[14] = (bytes[0]     << 4)          | (bytes[1]     >> 4);
    packet->payload[15] = (bytes[1]     << 4)          | (bytes[2]     >> 4);
    packet->payload[16] = (bytes[2]     << 4)          | (bytes[3]     >> 4);
    packet->payload[17] = (bytes[3]     << 4)          | (emb.bytes[1] >> 4);
    packet->payload[18] = (emb.bytes[1] << 4)          | (packet->payload[18] & 0x0f);

    return 0;
}
