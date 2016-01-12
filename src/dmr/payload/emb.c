#include <string.h>
#ifdef DMR_DEBUG
#include <assert.h>
#endif
#include "dmr/error.h"
#include "dmr/payload.h"
#include "dmr/fec/qr_16_7.h"


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
