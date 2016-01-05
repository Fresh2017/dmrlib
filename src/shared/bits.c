#include <stdlib.h>

#include "dmr/bits.h"

uint8_t bits_to_byte(bit_t bits[8])
{
    uint8_t val = 0;
    for (uint8_t i = 0; i < 8; i++) {
        if (bits[i])
            val |= (1 << (7 - i));
    }
    return val;
}

void bits_to_bytes(bit_t *bits, size_t bits_length, uint8_t *bytes, size_t bytes_length)
{
    for (uint16_t i = 0; i < min(bits_length/8, bytes_length); i++)
        bytes[i] = bits_to_byte(&bits[i * 8]);
}

void byte_to_bits(uint8_t byte, bit_t bits[8]) {
	bits[0] = (byte & 128) > 0;
	bits[1] = (byte & 64) > 0;
	bits[2] = (byte & 32) > 0;
	bits[3] = (byte & 16) > 0;
	bits[4] = (byte & 8) > 0;
	bits[5] = (byte & 4) > 0;
	bits[6] = (byte & 2) > 0;
	bits[7] = (byte & 1) > 0;
}

void bytes_to_bits(uint8_t *bytes, size_t bytes_length, bit_t *bits, size_t bits_length)
{
    for (uint16_t i = 0; i < min(bits_length/8, bytes_length); i++)
        byte_to_bits(bytes[i], &bits[i * 8]);
}
