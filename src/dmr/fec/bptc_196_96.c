#include <string.h>
#include <stdio.h>

#include "dmr/error.h"
#include "dmr/log.h"
#include "dmr/fec/bptc_196_96.h"
#include "dmr/fec/hamming.h"

#if defined(DMR_DEBUG_BPTC)
static void bptc_196_96_dump(dmr_bptc_196_96 *bptc)
{
	uint8_t row, col;

	printf("BPTC(196,96): matrix:\n");
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
			if (bptc->deinterleaved_bits[(row * 15) + col /* + 1 */]) {
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
	fflush(stdout);
}
#endif // DMR_DEBUG

int dmr_bptc_196_96_decode(dmr_packet packet, dmr_bptc_196_96 *bptc, uint8_t data[12])
{
	if (bptc == NULL || packet == NULL || data == NULL)
		return dmr_error(DMR_EINVAL);

	dmr_log_trace("BPTC(196,96): decode");
	uint8_t row, col, i;
	bool bits[DMR_PACKET_BITS], data_bits[96];

	dmr_bytes_to_bits(packet, DMR_PACKET_LEN, bits, DMR_PACKET_BITS);
	memcpy(bptc->raw +  0, bits +   0, 98);
	memcpy(bptc->raw + 98, bits + 166, 98);
	//dmr_dump_hex(packet->payload, 96/8);
	//dmr_dump_hex(bptc->raw, 196);

	// Deinterleaver
	for (i = 1; i < 197; i++) {
		bptc->deinterleaved_bits[i - 1] = bptc->raw[(i * 181) % 196];
	}

#if defined(DMR_DEBUG_BPTC)
	bptc_196_96_dump(bptc);
#endif

	for (col = 0; col < 15; col++) {
		bool data[13];
		for (row = 0; row < 13; row++) {
			data[row] = bptc->deinterleaved_bits[(row * 15) + 1];
		}
		if (!dmr_hamming_13_9_3_decode(data)) {
			return -1;
		}
		for (row = 0; row < 13; row++) {
			bptc->deinterleaved_bits[(row * 15) + 1] = data[row];
		}
	}

	for (row = 0; row < 9; row++) {
		bool data[15];
		for (col = 0; col < 15; col++) {
			data[col] = bptc->deinterleaved_bits[(row * 15) + col];
		}
		if (!dmr_hamming_15_11_3_decode(data)) {
			return -1;
		}
		for (col = 0; col < 11; col++) {
			bptc->deinterleaved_bits[(row * 15) + col] = data[col];
		}
	}

	for (col = 3, i = 0; col < 11; col++, i++) {
		data_bits[i] = bptc->deinterleaved_bits[(0 * 15) + col];
	}
	for (row = 1; row < 9; row++) {
		for (col = 0; col < 11; col++, i++) {
			//dmr_log_trace("copy bit %u to %u", (row * 15) + col, i);
			data_bits[i] = bptc->deinterleaved_bits[(row * 15) + col];
		}
	}

	dmr_bits_to_bytes(data_bits, 96, data, 12);
	return 0;
}

int dmr_bptc_196_96_encode(dmr_packet packet, dmr_bptc_196_96 *bptc, uint8_t data[12])
{
	if (bptc == NULL || packet == NULL || data == NULL)
		return dmr_error(DMR_EINVAL);

	dmr_log_trace("BPTC(196,96): encode");
	uint8_t row, col, i;
	bool data_bits[96], bits[DMR_PACKET_BITS], hc[15];

	dmr_bytes_to_bits(data, 12, data_bits, 96);
	memset(bptc->deinterleaved_bits, 0, sizeof(bptc->deinterleaved_bits));

	i = 0;
	for (row = 0; row < 9; row++) {
		memset(hc, 0, 15);
		if (row == 0) {
			for (col = 3; col < 11; col++) {
				hc[col] = bptc->deinterleaved_bits[col] = data_bits[i++];
			}
		} else {
			for (col = 0; col < 11; col++) {
				hc[col] = bptc->deinterleaved_bits[(row * 15) + col] = data_bits[i++];
			}
		}
		dmr_hamming_15_11_3_encode(hc);
		for (col = 11; col < 15; col++) {
			bptc->deinterleaved_bits[(row * 15) + col] = hc[col];
		}
	}
	for (col = 0; col < 15; col++) {
		for (row = 0; row < 9; row++) {
			hc[row] = bptc->deinterleaved_bits[(row * 15) + col];
		}
		dmr_hamming_13_9_3_encode(hc);
		bptc->deinterleaved_bits[col + 135] = hc[9];
		bptc->deinterleaved_bits[col + 135 + 15] = hc[10];
		bptc->deinterleaved_bits[col + 135 + 30] = hc[11];
		bptc->deinterleaved_bits[col + 135 + 45] = hc[12];
	}

#if defined(DMR_DEBUG_BPTC)
	bptc_196_96_dump(bptc);
#endif

	// Interleaver
	for (i = 1; i < 197; i++) {
		bptc->raw[(i * 181) % 196] = bptc->deinterleaved_bits[i - 1];
	}

	memcpy(bits +   0, bptc->raw +  0, 98);
	memcpy(bits + 166, bptc->raw + 98, 98);
	dmr_bits_to_bytes(bits, DMR_PACKET_BITS, packet, DMR_PACKET_LEN);

	return 0;
}
