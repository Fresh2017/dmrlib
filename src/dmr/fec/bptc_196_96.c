#include <string.h>
#include <stdio.h>

#include "dmr/error.h"
#include "dmr/log.h"
#include "dmr/fec/bptc_196_96.h"
#include "dmr/fec/hamming.h"

#if defined(DMR_DEBUG)
static void bptc_196_96_dump(dmr_bptc_196_96_t *bptc)
{
	uint8_t row, col;

	printf("bptc_196_96: matrix:\n");
	for (row = 0; row < 13; row++) {
		if (row == 0) {
			printf("    | ");
			for (col = 0; col < 15; col++) {
				if (col == 11) {
					printf(" | ");
				}
				printf(" %02d", col);
			}
			putchar('\n');
			printf("----+-------------------------------------------------\n");
		}
		printf("#%02d | ", row);
		for (col = 0; col < 15; col++) {
			if (col == 11) {
				printf(" | ");
			}
			// +1 because the first bit is R(3) and it's not used
			if (bptc->deinterleaved_bits[(row * 15) + col + 1]) {
				printf(" 1 ");
			} else {
				printf(" 0 ");
			}
		}
		putchar('\n');
		if (row == 8) {
			printf("----+-------------------------------------------------\n");
		}
	}
}
#endif

static void bptc_196_96_decode(dmr_bptc_196_96_t *bptc, dmr_packet_t *packet)
{
	dmr_byte_to_bits(packet->payload[ 0], bptc->raw + 0);
	dmr_byte_to_bits(packet->payload[ 1], bptc->raw + 8);
	dmr_byte_to_bits(packet->payload[ 2], bptc->raw + 16);
	dmr_byte_to_bits(packet->payload[ 3], bptc->raw + 24);
	dmr_byte_to_bits(packet->payload[ 4], bptc->raw + 32);
	dmr_byte_to_bits(packet->payload[ 5], bptc->raw + 40);
	dmr_byte_to_bits(packet->payload[ 6], bptc->raw + 48);
	dmr_byte_to_bits(packet->payload[ 7], bptc->raw + 56);
	dmr_byte_to_bits(packet->payload[ 8], bptc->raw + 64);
	dmr_byte_to_bits(packet->payload[ 9], bptc->raw + 72);
	dmr_byte_to_bits(packet->payload[10], bptc->raw + 80);
	dmr_byte_to_bits(packet->payload[11], bptc->raw + 88);
	dmr_byte_to_bits(packet->payload[12], bptc->raw + 96);
	bool bits[8];
	dmr_byte_to_bits(packet->payload[20], bits);
	bptc->raw[98] = bits[6];
	bptc->raw[99] = bits[7];
	dmr_byte_to_bits(packet->payload[21], bptc->raw + 100);
	dmr_byte_to_bits(packet->payload[22], bptc->raw + 108);
	dmr_byte_to_bits(packet->payload[23], bptc->raw + 116);
	dmr_byte_to_bits(packet->payload[24], bptc->raw + 124);
	dmr_byte_to_bits(packet->payload[25], bptc->raw + 132);
	dmr_byte_to_bits(packet->payload[26], bptc->raw + 140);
	dmr_byte_to_bits(packet->payload[27], bptc->raw + 148);
	dmr_byte_to_bits(packet->payload[28], bptc->raw + 156);
	dmr_byte_to_bits(packet->payload[29], bptc->raw + 164);
	dmr_byte_to_bits(packet->payload[30], bptc->raw + 172);
	dmr_byte_to_bits(packet->payload[31], bptc->raw + 180);
	dmr_byte_to_bits(packet->payload[32], bptc->raw + 188);
}

static void bptc_196_96_deinterleave(dmr_bptc_196_96_t *bptc)
{
	uint16_t i, j;
	memset(bptc->deinterleaved_bits, 0, sizeof(bptc->deinterleaved_bits));

	for (i = 0; i < 196; i++) {
		j = (i * 181) % 196;
		bptc->deinterleaved_bits[i] = bptc->raw[j];
	}
}

static bool bptc_196_96_decode_parity(dmr_bptc_196_96_t *bptc)
{
	bool retry;
	uint8_t attempt = 0, col, row, pos, i;
	do {
		retry = false;
		
		// Run through each of the 15 columns
		bool cbits[13];
		for (col = 0; col < 15; col++) {
			pos = col + 1;
			for (i = 0; i < 13; i++) {
				cbits[i] = bptc->deinterleaved_bits[pos];
				pos += 15;
			}

			if (dmr_hamming_13_9_3_verify_bits(cbits)) {
				dmr_log_debug("bptc_196_96: hamming(13, 9, 3) bit errors in col #%u", col);
				pos = col + 1;
				for (i = 0; i < 13; i++) {
					bptc->deinterleaved_bits[pos] = cbits[i];
					pos += 15;
				}
				retry = true;
			}
		}

		// Run through each of the 9 rows containing data
		for (row = 0; row < 9; row++) {
			pos = (row * 15) + 1;
			if (dmr_hamming_15_11_3_verify_bits(bptc->deinterleaved_bits + pos)) {
				dmr_log_debug("bptc_196_96: hamming(15, 11, 3) bit errors in row #%u", row);
				retry = true;
			}
		}

		attempt++;
	} while (retry && attempt < 5);

	if (attempt > 1) {
		dmr_log_debug("bptc_196_96: bit errors corrected in %u attempts", attempt);
	}

	return attempt < 5;
}

static void bptc_196_96_decode_data(dmr_bptc_196_96_t *bptc, uint8_t data[12])
{
	bool bits[96];
	memset(bits, 0, sizeof(bits));

	uint8_t i, j = 0;
	for (i = 4; i < 12; i++, j++)
		bits[j] = bptc->deinterleaved_bits[i];
	for (i = 16; i < 27; i++, j++)
		bits[j] = bptc->deinterleaved_bits[i];
	for (i = 31; i < 42; i++, j++)
		bits[j] = bptc->deinterleaved_bits[i];
	for (i = 46; i < 57; i++, j++)
		bits[j] = bptc->deinterleaved_bits[i];
	for (i = 61; i < 72; i++, j++)
		bits[j] = bptc->deinterleaved_bits[i];
	for (i = 76; i < 87; i++, j++)
		bits[j] = bptc->deinterleaved_bits[i];
	for (i = 91; i < 102; i++, j++)
		bits[j] = bptc->deinterleaved_bits[i];
	for (i = 106; i < 117; i++, j++)
		bits[j] = bptc->deinterleaved_bits[i];
	for (i = 121; i < 131; i++, j++)
		bits[j] = bptc->deinterleaved_bits[i];

	dmr_bits_to_bytes(bits, 96, data, 12);
}

int dmr_bptc_196_96_decode(dmr_bptc_196_96_t *bptc, dmr_packet_t *packet, uint8_t data[12])
{
	if (bptc == NULL || packet == NULL || data == NULL)
		return dmr_error(DMR_EINVAL);

	dmr_log_trace("bptc_196_96: decode");

	// Convert to bits
	bptc_196_96_decode(bptc, packet);

	// Deinterleave bits
	bptc_196_96_deinterleave(bptc);

#if defined(DMR_DEBUG)
	bptc_196_96_dump(bptc);
#endif

	// Decode and check parity
	if (!bptc_196_96_decode_parity(bptc)) {
		memset(data, 0, sizeof(uint8_t) * 12);
		return dmr_error_set("bptc_196_96: parity check failed");
	}

	// Convert deinterleaved bits to output data (12 bytes / 96 bits)
	bptc_196_96_decode_data(bptc, data);

	return 0;
}

static void bptc_196_96_encode_data(dmr_bptc_196_96_t *bptc, uint8_t data[12])
{
	bool bits[96];
	uint8_t i, j = 0;
	
	// All 12 bytes
	for (i = 0; i < 12; i++)
		dmr_byte_to_bits(*(data + i), bits + (i << 3));

	// Store deinterleaved bits	
	memset(bptc->deinterleaved_bits, 0, sizeof(bptc->deinterleaved_bits));
#define C(a, b) do { for (i = a; i < b; i++, j++) bptc->deinterleaved_bits[i] = bits[j]; } while(0)
	C(  4,  12);
	C( 16,  27);
	C( 31,  42);
	C( 46,  57);
	C( 61,  72);
	C( 76,  87);
	C( 91, 102);
	C(106, 117);
	C(121, 132);
#undef C
}

static void bptc_196_96_encode_parity(dmr_bptc_196_96_t *bptc)
{
	bool bits[196] = { 0, }, cbits[13] = { 0, };
	uint8_t i, col, row, pos = 0;
	
	// For all 15 columns
	for (col = 0; col < 15; col++) {
		pos = col + 1;
		for (i = 0; i < 13; i++) {
			cbits[i] = bptc->deinterleaved_bits[pos];
			pos += 15;
		}
		dmr_hamming_13_9_3_encode_bits(cbits, cbits + 9);
		
		pos = col + 1;
		for (i = 0; i < 13; i++) {
			bptc->deinterleaved_bits[pos] = cbits[i];
			pos += 15;
		}
	}

	// For all 9 rows
	for (row = 0; row < 9; row++) {
		pos = (row * 15) + 1;
		dmr_hamming_15_11_3_encode_bits(
			bptc->deinterleaved_bits + pos,
			bptc->deinterleaved_bits + pos + 11);
	}
	
	for (col = 0; col < 15; col++) {
		for (row = 0; row < 9; row++) {
			cbits[row] = bptc->deinterleaved_bits[(row * 15) + 1];
		}
		dmr_hamming_13_9_3_encode_bits(cbits, cbits + 9);
		bptc->deinterleaved_bits[col + 135 + 1 +  0] = cbits[ 9];
		bptc->deinterleaved_bits[col + 135 + 1 + 15] = cbits[10];
		bptc->deinterleaved_bits[col + 135 + 1 + 30] = cbits[11];
		bptc->deinterleaved_bits[col + 135 + 1 + 45] = cbits[12];
	}

	/*
	for (row = 0; row < 9; row++) {
		if (row == 0) {
			for (col = 3; col < 11; col++) {
				bits[col+1] = bptc->deinterleaved_bits[pos++];
			}
		} else {
			for (col = 0; col < 11; col++) {
				bits[(row * 15) + col +1] = bptc->deinterleaved_bits[pos++];
			}
		}
		dmr_hamming_15_11_3_encode_bits(bits + (row * 15) + 1);
	}

	for (col = 0; col < 15; col++) {
		for (row = 0; row < 9; row++) {
			cbits[row] = bits[(row * 15) + col + 1];
		}

		dmr_hamming_13_9_3_encode_bits(cbits);
		bits[col + 135 + 1 +  0] = cbits[ 9];
		bits[col + 135 + 1 + 15] = cbits[10];
		bits[col + 135 + 1 + 30] = cbits[11];
		bits[col + 135 + 1 + 45] = cbits[12];
	}
	
	memcpy(bptc->deinterleaved_bits, bits, sizeof(bits));
	*/
}

static void bptc_196_96_interleave(dmr_bptc_196_96_t *bptc)
{
	uint16_t i, j;
	memset(bptc->raw, 0, sizeof(bptc->raw));

	for (i = 0; i < 196; i++) {
		j = (i * 181) % 196;
		bptc->raw[j] = bptc->deinterleaved_bits[i];
	}
}

static void bptc_196_96_encode(dmr_bptc_196_96_t *bptc, dmr_packet_t *packet)
{
	// First 104 bits
	packet->payload[ 0] = dmr_bits_to_byte(bptc->raw +  0);
	packet->payload[ 1] = dmr_bits_to_byte(bptc->raw +  8);
	packet->payload[ 2] = dmr_bits_to_byte(bptc->raw + 16);
	packet->payload[ 3] = dmr_bits_to_byte(bptc->raw + 24);
	packet->payload[ 4] = dmr_bits_to_byte(bptc->raw + 32);
	packet->payload[ 5] = dmr_bits_to_byte(bptc->raw + 40);
	packet->payload[ 6] = dmr_bits_to_byte(bptc->raw + 48);
	packet->payload[ 7] = dmr_bits_to_byte(bptc->raw + 56);
	packet->payload[ 8] = dmr_bits_to_byte(bptc->raw + 64);
	packet->payload[ 9] = dmr_bits_to_byte(bptc->raw + 72);
	packet->payload[10] = dmr_bits_to_byte(bptc->raw + 80);
	packet->payload[11] = dmr_bits_to_byte(bptc->raw + 88);
	// Last 4 bits of first half and first 4 bits of last half
	uint8_t byte = dmr_bits_to_byte(bptc->raw + 96);
	packet->payload[12] = (packet->payload[12] & 0x3f) | ((byte >> 0) & 0xc0);
	packet->payload[13] = (packet->payload[13] & 0xfc) | ((byte >> 4) & 0x03);
	// Last 104 bits
	packet->payload[21] = dmr_bits_to_byte(bptc->raw + 100);
	packet->payload[22] = dmr_bits_to_byte(bptc->raw + 108);
	packet->payload[23] = dmr_bits_to_byte(bptc->raw + 116);
	packet->payload[24] = dmr_bits_to_byte(bptc->raw + 124);
	packet->payload[25] = dmr_bits_to_byte(bptc->raw + 132);
	packet->payload[26] = dmr_bits_to_byte(bptc->raw + 140);
	packet->payload[27] = dmr_bits_to_byte(bptc->raw + 148);
	packet->payload[28] = dmr_bits_to_byte(bptc->raw + 156);
	packet->payload[29] = dmr_bits_to_byte(bptc->raw + 164);
	packet->payload[30] = dmr_bits_to_byte(bptc->raw + 172);
	packet->payload[31] = dmr_bits_to_byte(bptc->raw + 180);
	packet->payload[32] = dmr_bits_to_byte(bptc->raw + 188);
}

int dmr_bptc_196_96_encode(dmr_bptc_196_96_t *bptc, dmr_packet_t *packet, uint8_t data[12])
{
	if (bptc == NULL || packet == NULL || data == NULL)
		return dmr_error(DMR_EINVAL);

	dmr_log_trace("bptc_196_96: encode");

	// Convert input data (12 bytes / 96 bits) to deinterleaved bits
	bptc_196_96_encode_data(bptc, data);

#if defined(DMR_DEBUG)
	bptc_196_96_dump(bptc);
#endif

	// Add Hamming parity
	bptc_196_96_encode_parity(bptc);

	// Interleave
	bptc_196_96_interleave(bptc);

	// Convert back to bytes
	bptc_196_96_encode(bptc, packet);

	return 0;
}
