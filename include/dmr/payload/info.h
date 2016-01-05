#ifndef _DMR_PAYLOAD_INFO_H
#define _DMR_PAYLOAD_INFO_H

#include <dmr/type.h>

#define DMR_PAYLOAD_INFO_HALF        98
#define DMR_PAYLOAD_INFO_BITS        (DMR_PAYLOAD_INFO_HALF*2)

typedef struct {
	bit_t bits[DMR_PAYLOAD_INFO_BITS];
} dmr_payload_info_bits_t;

extern dmr_payload_info_bits_t *dmr_payload_get_info_bits(dmr_payload_t *payload);
extern dmr_payload_info_bits_t *dmr_info_bits_deinterleave(dmr_payload_info_bits_t *info_bits);

#endif // _DMR_PAYLOAD_INFO_H
