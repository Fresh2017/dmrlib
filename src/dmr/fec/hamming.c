#include <inttypes.h>
#include "dmr/fec/hamming.h"

bool hamming_parity_errors(bool *parities, uint8_t len)
{
	uint8_t i;
	for (i = 0; i < len; i++) {
		if (parities[i])
			return true;
	}
	return false;
}

void dmr_hamming_13_9_3_encode_bits(bool bits[9], bool parity[4])
{
	parity[0] = (bits[0] ^ bits[1] ^ bits[3] ^ bits[5] ^ bits[6]);
	parity[1] = (bits[0] ^ bits[1] ^ bits[2] ^ bits[4] ^ bits[6] ^ bits[7]);
	parity[2] = (bits[0] ^ bits[1] ^ bits[2] ^ bits[3] ^ bits[5] ^ bits[7] ^ bits[8]);
	parity[3] = (bits[0] ^ bits[2] ^ bits[4] ^ bits[5] ^ bits[8]);
}

bool dmr_hamming_13_9_3_verify_bits(bool bits[13], bool parity[4])
{
	dmr_hamming_13_9_3_encode_bits(bits, parity);
	parity[0] ^= bits[ 9];
	parity[1] ^= bits[10];
	parity[2] ^= bits[11];
	parity[3] ^= bits[12];
	return !hamming_parity_errors(parity, 4);
}

void dmr_hamming_15_11_3_encode_bits(bool bits[11], bool parity[4])
{
	parity[0] = bits[0] ^ bits[1] ^ bits[2] ^ bits[3] ^ bits[5] ^ bits[7] ^ bits[ 8];
	parity[1] = bits[1] ^ bits[2] ^ bits[3] ^ bits[4] ^ bits[6] ^ bits[8] ^ bits[ 9];
	parity[2] = bits[2] ^ bits[3] ^ bits[4] ^ bits[5] ^ bits[7] ^ bits[9] ^ bits[10];
	parity[3] = bits[0] ^ bits[1] ^ bits[2] ^ bits[4] ^ bits[6] ^ bits[7] ^ bits[10];
}

bool dmr_hamming_15_11_3_verify_bits(bool bits[15], bool parity[4])
{
	dmr_hamming_15_11_3_encode_bits(bits, parity);
	parity[0] ^= bits[11];
	parity[1] ^= bits[12];
	parity[2] ^= bits[13];
	parity[3] ^= bits[14];
	return !hamming_parity_errors(parity, 4);
}

void dmr_hamming_16_11_4_encode_bits(bool bits[11], bool parity[5])
{
	parity[0] = bits[0] ^ bits[1] ^ bits[2] ^ bits[3] ^ bits[5] ^ bits[7] ^ bits[8];
	parity[1] = bits[1] ^ bits[2] ^ bits[3] ^ bits[4] ^ bits[6] ^ bits[8] ^ bits[9];
	parity[2] = bits[2] ^ bits[3] ^ bits[4] ^ bits[5] ^ bits[7] ^ bits[9] ^ bits[10];
	parity[3] = bits[0] ^ bits[1] ^ bits[2] ^ bits[4] ^ bits[6] ^ bits[7] ^ bits[10];
	parity[4] = bits[0] ^ bits[2] ^ bits[5] ^ bits[6] ^ bits[8] ^ bits[9] ^ bits[10];
}

bool dmr_hamming_16_11_4_verify_bits(bool bits[16], bool parity[5])
{
	dmr_hamming_16_11_4_encode_bits(bits, parity);
	parity[0] ^= bits[11];
	parity[1] ^= bits[12];
	parity[2] ^= bits[13];
	parity[3] ^= bits[14];
	parity[4] ^= bits[15];
	return !hamming_parity_errors(parity, 5);
}

void dmr_hamming_17_12_3_encode_bits(bool bits[12], bool parity[5])
{
	parity[0] = bits[0] ^ bits[1] ^ bits[2] ^ bits[3] ^ bits[6] ^ bits[ 7] ^ bits[9];
	parity[1] = bits[0] ^ bits[1] ^ bits[2] ^ bits[3] ^ bits[4] ^ bits[ 7] ^ bits[8] ^ bits[10];
	parity[2] = bits[1] ^ bits[2] ^ bits[3] ^ bits[4] ^ bits[5] ^ bits[ 8] ^ bits[9] ^ bits[11];
	parity[3] = bits[0] ^ bits[1] ^ bits[4] ^ bits[5] ^ bits[7] ^ bits[10];
	parity[4] = bits[0] ^ bits[1] ^ bits[2] ^ bits[5] ^ bits[6] ^ bits[ 8] ^ bits[11];
}

bool dmr_hamming_17_12_3_verify_bits(bool bits[17], bool parity[5])
{
	dmr_hamming_17_12_3_encode_bits(bits, parity);
	parity[0] ^= bits[12];
	parity[1] ^= bits[13];
	parity[2] ^= bits[14];
	parity[3] ^= bits[15];
	parity[4] ^= bits[16];
	return !hamming_parity_errors(parity, 5);
}

uint8_t dmr_hamming_parity_position(bool *bits, uint8_t len)
{
	uint8_t i;
	for (i = 0; i < len; i++) {
		if (!bits[i])
			return i;
	}
	return 0xff;
}
