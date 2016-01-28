#include <inttypes.h>
#include "dmr/fec/hamming.h"
#include "dmr/bits.h"
#include "dmr/log.h"

typedef struct {
	uint8_t n; 		/* block length */
	uint8_t k; 		/* message length */
	uint8_t d; 		/* distance */
	uint8_t p;  	/* parity bits */
	uint8_t g[]; 	/* generator matrix */
} hamming_t;

static hamming_t hamming_7_4_3 = {
	.n = 7,
	.k = 4,
	.d = 3,
	.g = {
		0x05, // 0b101
		0x07, // 0b111
		0x06, // 0b110
		0x03, // 0b011
		0x04, // 0b100
		0x02, // 0b010
		0x01, // 0b001
	}
};

static hamming_t hamming_13_9_3 = {
	.n = 13,
	.k = 9,
	.d = 3,
	.g = {
		0x0f, // 0b1111
		0x0e, // 0b1110
		0x07, // 0b0111
		0x0a, // 0b1010
		0x05, // 0b0101
		0x0b, // 0b1011
		0x0c, // 0b1100
		0x06, // 0b0110
		0x03, // 0b0011
		0x08, // 0b1000
		0x04, // 0b0100
		0x02, // 0b0010
		0x01, // 0b0001
	}
};

static hamming_t hamming_15_11_3 = {
	.n = 15,
	.k = 11,
	.d = 3,
	.g = {
		0x09, // 0b1001
		0x0d, // 0b1101
		0x0f, // 0b1111
		0x0e, // 0b1110
		0x07, // 0b0111
		0x0a, // 0b1010
		0x05, // 0b0101
		0x0b, // 0b1011
		0x0c, // 0b1100
		0x06, // 0b0110
		0x03, // 0b0011
		0x08, // 0b1000
		0x04, // 0b0100
		0x02, // 0b0010
		0x01, // 0b0001
	}
};

static hamming_t hamming_16_11_4 = {
	.n = 16,
	.k = 11,
	.d = 4,
	.g = {
		0x13, // 0b10011
		0x1a, // 0b11010
		0x1f, // 0b11111
		0x1c, // 0b11100
		0x0e, // 0b01110
		0x15, // 0b10101
		0x0b, // 0b01011
		0x16, // 0b10110
		0x19, // 0b11001
		0x0d, // 0b01101
		0x07, // 0b00111
		0x10, // 0b10000
		0x08, // 0b01000
		0x04, // 0b00100
		0x02, // 0b00010
		0x01, // 0b00001
	}
};

static hamming_t hamming_17_12_3 = {
	.n = 17,
	.k = 12,
	.d = 3,
	.g = {
		0x1b, // 0b11011
		0x1f, // 0b11111
		0x1d, // 0b11101
		0x1c, // 0b11100
		0x0e, // 0b01110
		0x07, // 0b00111
		0x11, // 0b10001
		0x1a, // 0b11010
		0x0d, // 0b01101
		0x14, // 0b10100
		0x0a, // 0b01010
		0x05, // 0b00101
		0x10, // 0b10000
		0x08, // 0b01000
		0x04, // 0b00100
		0x02, // 0b00010
		0x01, // 0b00001
	}
};

static void hamming_parity(hamming_t *h, bool *d, bool *p)
{
	uint8_t x, y, b = h->n - h->k;
	for (x = 0; x < b; x++) {
		p[x] = 0;
		for (y = 0; y < h->k; y++) {
			if (h->g[y] & (1 << (b - x - 1))) {
				p[x] ^= d[y];
			}
		}
	}
}

static bool hamming_parity_check(hamming_t *h, bool *d)
{
	uint8_t i, pos = 0, b = h->n - h->k, err = 0;
	bool p[b];
	hamming_parity(h, d, p);

check_again:
	for (i = 0; i < b; i++) {
		if (d[i + h->k] != p[i]) {
			pos |= (1 << i);
		}
	}
	if (pos == 0) {
		return true;
	}

	// Check in our generator matrix what bit position this is for. Our parity
	// error position is 1-indexed, while our generator matrix is 0-indexed.
	pos = h->g[pos - 1];
	dmr_log_debug("Hamming(%u,%u,%u): parity error at bit %u",
		h->n, h->k, h->d, pos);

	// Keep trying until we hit our max distance
	if (++err < h->d) {
		goto check_again;
	}
	dmr_log_error("Hamming(%u,%u,%u): repair failed after %u attempts",
		h->n, h->k, h->d, err);
	return false;
}

#define HAMMING_CODE_DMR_NAME(b,fn) dmr_##b##_##fn
#define HAMMING_CODE(b,_s) 			bool HAMMING_CODE_DMR_NAME(b, decode)(bool d[_s]) { \
	dmr_log_trace("Hamming(%u,%u,%u): parity check", b.n, b.k, b.d); \
	return hamming_parity_check(&b, d); \
} \
\
void HAMMING_CODE_DMR_NAME(b, encode)(bool d[_s]) { \
	dmr_log_trace("Hamming(%u,%u,%u): encode", b.n, b.k, b.d); \
	return hamming_parity(&b, d, d + b.k); \
}

HAMMING_CODE(hamming_7_4_3, 7);
HAMMING_CODE(hamming_13_9_3, 13);
HAMMING_CODE(hamming_15_11_3, 15);
HAMMING_CODE(hamming_16_11_4, 16);
HAMMING_CODE(hamming_17_12_3, 17);
