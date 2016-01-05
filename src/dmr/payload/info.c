#include <string.h>
#include <stdio.h>

#include "dmr/payload.h"

dmr_payload_info_bits_t *dmr_payload_get_info_bits(dmr_payload_t *payload)
{
    static dmr_payload_info_bits_t info_bits;

    if (payload == NULL)
        return NULL;

    memcpy(&info_bits.bits, payload->bits, DMR_PAYLOAD_INFO_HALF);
    memcpy(&info_bits.bits[DMR_PAYLOAD_INFO_HALF], payload->bits+98+10+48+10, DMR_PAYLOAD_INFO_HALF);

    return &info_bits;
}

dmr_payload_info_bits_t *dmr_info_bits_deinterleave(dmr_payload_info_bits_t *info_bits)
{
    static dmr_payload_info_bits_t deinterleaved_bits;

    if (info_bits == NULL)
        return NULL;

    for (uint8_t i = 0; i < DMR_PAYLOAD_INFO_BITS; i++)
        deinterleaved_bits.bits[i] = info_bits->bits[(i * 181) % DMR_PAYLOAD_INFO_BITS];

    return &deinterleaved_bits;
}
