#include "dmr/fec/hamming.h"

void dmr_hamming_13_9_3_encode_bits(bool bits[13])
{
	bits[9]  = bits[0] ^ bits[1] ^ bits[3] ^ bits[5] ^ bits[6];
	bits[10] = bits[0] ^ bits[1] ^ bits[2] ^ bits[4] ^ bits[6] ^ bits[7];
	bits[11] = bits[0] ^ bits[1] ^ bits[2] ^ bits[3] ^ bits[5] ^ bits[7] ^ bits[8];
	bits[12] = bits[0] ^ bits[2] ^ bits[4] ^ bits[5] ^ bits[8];
}

void dmr_hamming_15_11_3_encode_bits(bool bits[15])
{
	bits[11] = bits[0] ^ bits[1] ^ bits[2] ^ bits[3] ^ bits[5] ^ bits[7] ^ bits[8];
	bits[12] = bits[1] ^ bits[2] ^ bits[3] ^ bits[4] ^ bits[6] ^ bits[8] ^ bits[9];
	bits[13] = bits[2] ^ bits[3] ^ bits[4] ^ bits[5] ^ bits[7] ^ bits[9] ^ bits[10];
	bits[14] = bits[0] ^ bits[1] ^ bits[2] ^ bits[4] ^ bits[6] ^ bits[7] ^ bits[10];
}