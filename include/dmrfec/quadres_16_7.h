#ifndef _DMRFEC_QUADRES_16_7_H
#define _DMRFEC_QUADRES_16_7_H

#include <dmr/bits.h>

typedef struct {
	bit_t data[7];
	bit_t parity[9];
} dmrfec_quadres_16_7_codeword_t;

typedef struct {
	bit_t bits[9];
} dmrfec_quadres_16_7_parity_bits_t;

extern dmrfec_quadres_16_7_parity_bits_t *dmrfec_quadres_16_7_get_parity_bits(bit_t bits[7]);
extern bit_t dmrfec_quadres_16_7_check(dmrfec_quadres_16_7_codeword_t *codeword);
extern void dmrfec_quadres_16_7_init(void);

#endif // _DMRFEC_QUADRES_16_7_H
