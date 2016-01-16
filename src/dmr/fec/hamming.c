#include <inttypes.h>
#include "dmr/fec/hamming.h"

void dmr_hamming_13_9_3_encode_bits(bool bits[9], bool parity[4])
{
	parity[0] = bits[0] ^ bits[1] ^ bits[3] ^ bits[5] ^ bits[6];
	parity[1] = bits[0] ^ bits[1] ^ bits[2] ^ bits[4] ^ bits[6] ^ bits[7];
	parity[2] = bits[0] ^ bits[1] ^ bits[2] ^ bits[3] ^ bits[5] ^ bits[7] ^ bits[8];
	parity[3] = bits[0] ^ bits[2] ^ bits[4] ^ bits[5] ^ bits[8];	
}

bool dmr_hamming_13_9_3_verify_bits(bool bits[13])
{
	bool check[4];
	dmr_hamming_13_9_3_encode_bits(bits, check);

	uint8_t i = 0;
	if (check[0] != bits[ 9]) i |= 0x01;
	if (check[1] != bits[10]) i |= 0x02;
	if (check[2] != bits[11]) i |= 0x04;
	if (check[3] != bits[12]) i |= 0x08;

	switch (i) {
	// Parity bit errors
	case 0x01: bits[ 9] = !bits[ 9]; return true;
	case 0x02: bits[10] = !bits[10]; return true;
	case 0x04: bits[11] = !bits[11]; return true;
	case 0x08: bits[12] = !bits[12]; return true;

	// Data bit erros
	case 0x0F: bits[ 0] = !bits[ 0]; return true;
	case 0x07: bits[ 1] = !bits[ 1]; return true;
	case 0x0E: bits[ 2] = !bits[ 2]; return true;
	case 0x05: bits[ 3] = !bits[ 3]; return true;
	case 0x0A: bits[ 4] = !bits[ 4]; return true;
	case 0x0D: bits[ 5] = !bits[ 5]; return true;
	case 0x03: bits[ 6] = !bits[ 6]; return true;
	case 0x06: bits[ 7] = !bits[ 7]; return true;
	case 0x0C: bits[ 8] = !bits[ 8]; return true;

	// No bit errors
	default: return false;
	}
}

void dmr_hamming_15_11_3_encode_bits(bool bits[11], bool parity[4])
{
	parity[0] = bits[0] ^ bits[1] ^ bits[2] ^ bits[3] ^ bits[5] ^ bits[7] ^ bits[8];
	parity[1] = bits[1] ^ bits[2] ^ bits[3] ^ bits[4] ^ bits[6] ^ bits[8] ^ bits[9];
	parity[2] = bits[2] ^ bits[3] ^ bits[4] ^ bits[5] ^ bits[7] ^ bits[9] ^ bits[10];
	parity[3] = bits[0] ^ bits[1] ^ bits[2] ^ bits[4] ^ bits[6] ^ bits[7] ^ bits[10];
}

bool dmr_hamming_15_11_3_verify_bits(bool bits[15])
{
	bool check[4];
	dmr_hamming_15_11_3_encode_bits(bits, check);

	uint8_t i = 0;
	if (check[0] != bits[11]) i |= 0x01;
	if (check[1] != bits[12]) i |= 0x02;
	if (check[2] != bits[13]) i |= 0x04;
	if (check[3] != bits[14]) i |= 0x08;

	switch (i) {
	// Parity bit errors
	case 0x01: bits[11] = !bits[11]; return true;
	case 0x02: bits[12] = !bits[12]; return true;
	case 0x04: bits[13] = !bits[13]; return true;
	case 0x08: bits[14] = !bits[14]; return true;

	// Data bit errors
	case 0x09: bits[ 0] = !bits[ 0]; return true;
	case 0x0b: bits[ 1] = !bits[ 1]; return true;
	case 0x0f: bits[ 2] = !bits[ 2]; return true;
	case 0x07: bits[ 3] = !bits[ 3]; return true;
	case 0x0e: bits[ 4] = !bits[ 4]; return true;
	case 0x05: bits[ 5] = !bits[ 5]; return true;
	case 0x0a: bits[ 6] = !bits[ 6]; return true;
	case 0x0d: bits[ 7] = !bits[ 7]; return true;
	case 0x03: bits[ 8] = !bits[ 8]; return true;
	case 0x06: bits[ 9] = !bits[ 9]; return true;
	case 0x0c: bits[10] = !bits[10]; return true;

	// All good
	default: return false;
	}
}