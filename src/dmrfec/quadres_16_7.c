#include <string.h>

#include "dmrfec/quadres_16_7.h"

static dmrfec_quadres_16_7_parity_bits_t dmrfec_quadres_16_7_valid_data_paritys[128];

// Returns the quadratic residue (16,7,6) parity bits for the given byte.
dmrfec_quadres_16_7_parity_bits_t *dmrfec_quadres_16_7_get_parity_bits(bit_t bits[7]) {
	static dmrfec_quadres_16_7_parity_bits_t parity;

	// Multiplying the generator matrix with the given data bits.
	// See DMR AI spec. page 134.
	parity.bits[0] = bits[1] ^ bits[2] ^ bits[3] ^ bits[4];
	parity.bits[1] = bits[2] ^ bits[3] ^ bits[4] ^ bits[5];
	parity.bits[2] = bits[0] ^ bits[3] ^ bits[4] ^ bits[5] ^ bits[6];
	parity.bits[3] = bits[2] ^ bits[3] ^ bits[5] ^ bits[6];
	parity.bits[4] = bits[1] ^ bits[2] ^ bits[6];
 	parity.bits[5] = bits[0] ^ bits[1] ^ bits[4];
	parity.bits[6] = bits[0] ^ bits[1] ^ bits[2] ^ bits[5];
	parity.bits[7] = bits[0] ^ bits[1] ^ bits[2] ^ bits[3] ^ bits[6];
	parity.bits[8] = bits[0] ^ bits[2] ^ bits[4] ^ bits[5] ^ bits[6];

	return &parity;
}

static void dmrfec_quadres_16_7_calculate_valid_data_paritys(void) {
	uint16_t i;
	dmrfec_quadres_16_7_parity_bits_t *parity_bits = NULL;
	bit_t bits[9];

	for (i = 0; i < 128; i++) {
		byte_to_bits(i, bits);
		parity_bits = dmrfec_quadres_16_7_get_parity_bits(bits);
		memcpy(dmrfec_quadres_16_7_valid_data_paritys[i].bits, parity_bits->bits, 9);
	}
}

// Returns 1 if the codeword is valid.
bit_t dmrfec_quadres_16_7_check(dmrfec_quadres_16_7_codeword_t *codeword) {
	uint16_t col;
	uint8_t dataval = 0;

	if (codeword == NULL)
		return 0;

	for (col = 0; col < 7; col++) {
		if (codeword->data[col] == 1)
			dataval |= (1 << (7-col));
	}

	if (memcmp(dmrfec_quadres_16_7_valid_data_paritys[dataval].bits, codeword->parity, 9) == 0)
		return 1;

	return 0;
}

// Prefills the static data parity syndrome buffer with precalculated parities for each byte value.
void dmrfec_quadres_16_7_init(void) {
	dmrfec_quadres_16_7_calculate_valid_data_paritys();
}
