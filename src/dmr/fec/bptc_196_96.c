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
#endif // DMR_DEBUG

static void bptc_196_96_decode(dmr_bptc_196_96_t *bptc, dmr_packet_t *packet)
{
	/* First decode all 33 bytes to 264 bits */
	bool bits[DMR_PAYLOAD_BITS];
	dmr_bytes_to_bits(packet->payload, DMR_PAYLOAD_BYTES, bits, DMR_PAYLOAD_BITS);
	/* First 98 bits */
	memcpy(bptc->raw +  0, bits +   0, sizeof(bool) * 98);
	/* Second 98 bits, skipping over slot type (20 bits) and sync (48 bits) */
	memcpy(bptc->raw + 98, bits + 166, sizeof(bool) * 98);
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
	uint8_t attempt = 0, col, row, pos, i, err = 0;
	bool parity[4], cbits[13], ok = true;

	for (col = 0; col < 15; col++) {
		for (row = 0; row < 13; row++) {
			// +1 because the first bit is R(3) and it's not used so we can ignore that.
			cbits[row] = bptc->deinterleaved_bits[(row * 15) + col + 1];
		}
		if (!dmr_hamming_13_9_3_verify_bits(cbits, parity)) {
			err++;
			pos = dmr_hamming_parity_position(parity, 4);
			if (pos == 0xff) {
				ok = false;
				dmr_log_debug("bptc_196_96: Hamming (13,9,3) error, unrepairable error in col #%u", col);
			} else {
				dmr_log_trace("bptc_196_96: Hamming (13,9,3) error, fixing bit #%u in col #%u", pos, col);
				bptc->deinterleaved_bits[(pos * 15) + col + 1] = !bptc->deinterleaved_bits[(pos * 15) + col + 1];
				for (row = 0; row < 13; row++) {
					// +1 because the first bit is R(3) and it's not used so we can ignore that.
					cbits[row] = bptc->deinterleaved_bits[(row * 15) + col + 1];
				}
				if (!dmr_hamming_13_9_3_verify_bits(cbits, parity)) {
					ok = false;
					dmr_log_debug("bptc_196_96: Hamming (13,9,3) error, repair failed in col #%u", col);
				}
			}
		}
	}

	for (row = 0; row < 9; row++) {
		if (!dmr_hamming_15_11_3_verify_bits(bptc->deinterleaved_bits + (row * 15) + 1, parity)) {
			err++;
			pos = dmr_hamming_parity_position(parity, 4);
			if (pos == 0xff) {
				ok = false;
				dmr_log_debug("bptc_196_96: Hamming (15,11,3) error, unrepairable error in row #%u", row);
			} else {
				dmr_log_trace("bptc_196_96: Hamming (15,11,3) error, fixing bit #%u in row #%u", pos, row);
				bptc->deinterleaved_bits[(row * 15) + pos + 1] = !bptc->deinterleaved_bits[(row * 15) + pos + 1];
				if (!dmr_hamming_15_11_3_verify_bits(bptc->deinterleaved_bits + (row * 15) + 1, parity)) {
					ok = false;
					dmr_log_debug("bptc_196_96: Hamming (15,11,3) error, repair failed in row #%u", row);
				}
			}
		}
	}

	if (ok && err == 0) {
		dmr_log_debug("bptc_196_96: no errors found");
	} else if (ok) {
		dmr_log_debug("bptc_196_96: %u errors corrected", err);
	} else {
		dmr_log_error("bptc_196_96: unrecoverable errors");
	}

	return ok;
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
	for (i = 121; i < 132; i++, j++)
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
	bool parity[4], cbits[13];
	uint8_t i, col, row, pos = 0;

	/*
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
	*/

	pos = 0;
	for (row = 0; row < 9; row++) {
		if (row == 0) {
			for (col = 3; col < 11; col++) {
				// +1 because the first bit is R(3) and it's not used so we can ignore that.
				bptc->raw[col + 1] = bptc->deinterleaved_bits[pos++];
			}
		} else {
			for (col = 0; col < 11; col++) {
				// +1 because the first bit is R(3) and it's not used so we can ignore that.
				bptc->raw[col + (row * 15) + 1] = bptc->deinterleaved_bits[pos++];
			}
		}

		// +1 because the first bit is R(3) and it's not used so we can ignore that.
		dmr_hamming_15_11_3_encode_bits(bptc->raw + (row * 15) + 1, parity);
		bptc->raw[(row * 15) + 11 + 1] = parity[0];
		bptc->raw[(row * 15) + 12 + 1] = parity[1];
		bptc->raw[(row * 15) + 13 + 1] = parity[2];
		bptc->raw[(row * 15) + 14 + 1] = parity[3];
	}

	for (col = 0; col < 15; col++) {
		for (row = 0; row < 9; row++) {
			cbits[row] = bptc->raw[col+(row * 15)+1];
		}

		dmr_hamming_13_9_3_encode_bits(cbits, parity);
		bptc->raw[col + 135      + 1] = parity[0];
		bptc->raw[col + 135 + 15 + 1] = parity[1];
		bptc->raw[col + 135 + 30 + 1] = parity[2];
		bptc->raw[col + 135 + 45 + 1] = parity[3];
	}

	memcpy(bptc->deinterleaved_bits, bptc->raw, 196);
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
	/* First decode all 33 bytes to 264 bits */
	bool bits[DMR_PAYLOAD_BITS];
	dmr_bytes_to_bits(packet->payload, DMR_PAYLOAD_BYTES, bits, DMR_PAYLOAD_BITS);
	/* First 98 bits */
	memcpy(bits +   0, bptc->raw +  0, sizeof(bool) * 98);
	/* Second 98 bits, skipping over slot type (20 bits) and sync (48 bits) */
	memcpy(bits + 166, bptc->raw + 98, sizeof(bool) * 98);
	/* Convert back to bytes */
	dmr_bits_to_bytes(bits, DMR_PAYLOAD_BITS, packet->payload, DMR_PAYLOAD_BYTES);
}

int dmr_bptc_196_96_encode(dmr_bptc_196_96_t *bptc, dmr_packet_t *packet, uint8_t data[12])
{
	if (bptc == NULL || packet == NULL || data == NULL)
		return dmr_error(DMR_EINVAL);

	dmr_log_trace("bptc_196_96: encode");

	// Convert input data (12 bytes / 96 bits) to deinterleaved bits
	dmr_bytes_to_bits(data, 12, bptc->deinterleaved_bits, 96);

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
