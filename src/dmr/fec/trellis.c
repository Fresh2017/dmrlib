#include <string.h>
#include "dmr/fec/trellis.h"
#include "dmr/bits.h"
#include "dmr/error.h"

/* Table B.7: Trellis encoder state transition table */
static uint8_t state_transition_table[] = {
	0,  8, 4, 12, 2, 10, 6, 14,
	4, 12, 2, 10, 6, 14, 0,  8,
	1,  9, 5, 13, 3, 11, 7, 15,
	5, 13, 3, 11, 7, 15, 1,  9,
	3, 11, 7, 15, 1,  9, 5, 13,
	7, 15, 1,  9, 5, 13, 3, 11,
	2, 10, 6, 14, 0,  8, 4, 12,
	6, 14, 0,  8, 4, 12, 2, 10
};

/* Table B.8: Constellation to dibit pair mapping */
static int8_t constellation_point_mapping[16][2] = {
	{+1, -1}, {-1, -1}, {+3, -3}, {-3, -3},
	{-3, -1}, {+3, -1}, {-1, -3}, {+1, -3},
	{-3, +3}, {+3, +3}, {-1, +1}, {+1, +1},
	{+1, +3}, {-1, +3}, {+3, +1}, {-3, +1}
};

/* Table B.9: Interleaving schedule for rate 3⁄4 Trellis code */
static uint8_t rate_34_interleaving_schedule[98] = {
	0x00, 0x01, 0x08, 0x09, 0x10, 0x11, 0x18, 0x19, 0x20, 0x21, 0x28, 0x29,
	0x30, 0x31, 0x38, 0x39, 0x40, 0x41, 0x48, 0x49, 0x50, 0x51, 0x58, 0x59,
	0x60, 0x61, 0x02, 0x03, 0x0a, 0x0b, 0x12, 0x13, 0x1a, 0x1b, 0x22, 0x23,
	0x2a, 0x2b, 0x32, 0x33, 0x3a, 0x3b, 0x42, 0x43, 0x4a, 0x4b, 0x52, 0x53,
	0x5a, 0x5b, 0x04, 0x05, 0x0c, 0x0d, 0x14, 0x15, 0x1c, 0x1d, 0x24, 0x25,
	0x2c, 0x2d, 0x34, 0x35, 0x3c, 0x3d, 0x44, 0x45, 0x4c, 0x4d, 0x54, 0x55,
	0x5c, 0x5d, 0x06, 0x07, 0x0e, 0x0f, 0x16, 0x17, 0x1e, 0x1f, 0x26, 0x27,
	0x2e, 0x2f, 0x36, 0x37, 0x3e, 0x3f, 0x46, 0x47, 0x4e, 0x4f, 0x56, 0x57,
	0x5e, 0x5f
};

int dmr_trellis_rate_34_decode(dmr_packet packet, uint8_t bytes[18])
{
	if (packet == NULL || bytes == NULL)
		return dmr_error(DMR_EINVAL);

	/* Convert payload to bits and extract info bits */
	bool bits[DMR_PACKET_BITS], info[196];
	dmr_bytes_to_bits(packet, DMR_PACKET_LEN, bits, DMR_PACKET_BITS);
	memcpy(info +  0, bits +   0, 98);
	memcpy(info + 98, bits + 166, 98);

	/* Convert info bits to dibits */
	int8_t dibits[98], deinterleaved_dibits[98];
	uint8_t i, j, o;
	for (i = 0; i < 196; i += 2) {
		o = i / 2;
		if (bits[i]) {
			if (bits[i + 1]) {
				dibits[o] = -3;
			} else {
				dibits[o] = -1;
			}
		} else {
			if (bits[i + 1]) {
				dibits[o] = 3;
			} else {
				dibits[o] = 1;
			}
		}
	}

	/* Table B.9: Interleaving schedule for rate 3⁄4 Trellis code */
	for (i = 0; i < 98; i++) {
		deinterleaved_dibits[rate_34_interleaving_schedule[i]] = dibits[i];
	}

	/* Table B.8: Constellation to dibit pair mapping */
	uint8_t points[49];
	for (i = 0; i < 98; i += 2) {
		o = i / 2;
		for (j = 0; j < 16; j++) {
			if (dibits[i + 0] == constellation_point_mapping[j][0] &&
				dibits[i + 1] == constellation_point_mapping[j][1]) {
				points[o] = j;
				break;
			}
		}
	}

	/* Table B.7: Trellis encoder state transition table */
	uint8_t tribits[48], last = 0;
	bool match = false;
	int8_t start;
	for (i = 0; i < 48; i++) {
		start = last * 8;
		match = false;
		for (j = start; j < (start + 8); j++) {
			/* Check if this constellation point matches an element of this row of the state table */
			if (points[i] == state_transition_table[j]) {
				match = true;
				last = j - start;
				tribits[i] = last;
				break;
			}
		}
		if (!match) {
			return dmr_error_set("trellis: tribit extract error at point %u", i);
		}
	}

	/* Map 48 tribits back to 144 bits */
	memset(info, 0, sizeof(info));
	for (i = 0; i < 144; i += 3) {
		o = i / 3;
		if ((tribits[o] & 0x04) > 0) {
			bits[i + 0] = true;
		}
		if ((tribits[o] & 0x02) > 0) {
			bits[i + 1] = true;
		}
		if ((tribits[o] & 0x01) > 0) {
			bits[i + 2] = true;
		}
	}

	/* Map 144 bits back to bytes */
	dmr_bits_to_bytes(info, 144, bytes, 18);

	return 0;
}
