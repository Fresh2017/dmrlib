#include <string.h>

#include "dmr/payload.h"
#include "dmrfec/quadres_16_7.h"

dmr_emb_bits_t *dmr_emb_from_payload_sync_bits(dmr_payload_sync_bits_t *sync_bits)
{
    static dmr_emb_bits_t emb_bits;

    if (sync_bits == NULL)
        return NULL;

    memcpy(emb_bits.bits, sync_bits->bits, sizeof(dmr_emb_bits_t) / 2);
    memcpy(emb_bits.bits + 8, sync_bits->bits + sizeof(dmr_emb_bits_t) / 2 + sizeof(dmr_emb_signalling_lc_fragment_bits_t), sizeof(dmr_emb_bits_t) / 2);

    return &emb_bits;
}

dmr_emb_t *dmr_emb_decode(dmr_emb_bits_t *emb_bits)
{
    static dmr_emb_t emb;

    if (emb_bits == NULL)
        return NULL;

    if (!dmrfec_quadres_16_7_check((dmrfec_quadres_16_7_codeword_t *)emb_bits->bits))
        return NULL;

    // Privacy Indicator must be 0
    if (emb_bits->bits[4] != 0)
        return NULL;

    emb.color_code = emb_bits->bits[0] << 3 | emb_bits->bits[1] << 2 | emb_bits->bits[2] << 1 | emb_bits->bits[3];
    emb.lcss = emb_bits->bits[5] << 1 | emb_bits->bits[6];
    return &emb;
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
