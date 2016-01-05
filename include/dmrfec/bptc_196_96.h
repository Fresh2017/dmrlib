#ifndef _DMRFEC_BPTC_196_96_H
#define _DMRFEC_BPTC_196_96_H

#include <stdbool.h>
#include <dmr/bits.h>

typedef struct {
	bit_t bits[96];
} dmrfec_bptc_196_96_data_bits_t;

bool dmrfec_bptc_196_96_check_and_repair(bit_t deinterleaved_bits[196]);
dmrfec_bptc_196_96_data_bits_t *dmrfec_bptc_196_96_extractdata(bit_t deinterleaved_bits[196]);
//dmrpacket_payload_info_bits_t *dmrfec_bptc_196_96_generate(dmrfec_bptc_196_96_data_bits_t *data_bits);

#endif // _DMRFEC_BPTC_196_96_H
