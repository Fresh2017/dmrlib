#include <string.h>
#include <stdio.h>

#include "dmr/error.h"
#include "dmr/log.h"
#include "dmr/fec/bptc_196_96.h"

typedef struct {
	bool bits[4];
} dmr_bptc_196_96_error_vector_t;

static bool hamming_15_11_generator_matrix[] = {
	1, 0, 0, 1,
	1, 1, 0, 1,
	1, 1, 1, 1,
	1, 1, 1, 0,
	0, 1, 1, 1,
	1, 0, 1, 0,
	0, 1, 0, 1,
	1, 0, 1, 1,
	1, 1, 0, 0,
	0, 1, 1, 0,
	0, 0, 1, 1,

	1, 0, 0, 0, // These are used to determine errors in the Hamming checksum bits.
	0, 1, 0, 0,
	0, 0, 1, 0,
	0, 0, 0, 1
};

static bool hamming_13_9_generator_matrix[] = {
	1, 1, 1, 1,
	1, 1, 1, 0,
	0, 1, 1, 1,
	0, 1, 1, 1,
	0, 1, 0, 1,
	1, 0, 1, 1,
	1, 1, 0, 0,
	0, 1, 1, 0,
	0, 0, 1, 1,

	1, 0, 0, 0, // These are used to determine errors in the Hamming checksum bits.
	0, 1, 0, 0,
	0, 0, 1, 0,
	0, 0, 0, 1
};

// Hamming(15, 11, 3) checking of a matrix row (15 total bits, 11 data bits, min. distance: 3)
// See page 135 of the DMR Air Interface protocol specification for the generator matrix.
// A generator matrix looks like this: G = [Ik | P]. The parity check matrix is: H = [-P^T|In-k]
// In binary codes, then -P = P, so the negation is unnecessary. We can get the parity check matrix
// only by transposing the generator matrix. We then take a data row, and multiply it with each row
// of the parity check matrix, then xor each resulting row bits together with the corresponding
// parity check bit. The xor result (error vector) should be 0, if it's not, it can be used
// to determine the location of the erroneous bit using the generator matrix (P).
static void dmr_bptc_196_96_hamming_15_11_3_get_parity_bits(bool *data_bits, dmr_bptc_196_96_error_vector_t *error_vector) {
	if (data_bits == NULL || error_vector == NULL)
		return;

	error_vector->bits[0] = (data_bits[0] ^ data_bits[1] ^ data_bits[2] ^ data_bits[3] ^ data_bits[5] ^ data_bits[7] ^ data_bits[8]);
	error_vector->bits[1] = (data_bits[1] ^ data_bits[2] ^ data_bits[3] ^ data_bits[4] ^ data_bits[6] ^ data_bits[8] ^ data_bits[9]);
	error_vector->bits[2] = (data_bits[2] ^ data_bits[3] ^ data_bits[4] ^ data_bits[5] ^ data_bits[7] ^ data_bits[9] ^ data_bits[10]);
	error_vector->bits[3] = (data_bits[0] ^ data_bits[1] ^ data_bits[2] ^ data_bits[4] ^ data_bits[6] ^ data_bits[7] ^ data_bits[10]);
}

static bool dmr_bptc_196_96_hamming_15_11_3_errorcheck(bool *data_bits, dmr_bptc_196_96_error_vector_t *error_vector) {
	if (data_bits == NULL || error_vector == NULL)
		return 0;

	dmr_bptc_196_96_hamming_15_11_3_get_parity_bits(data_bits, error_vector);

	error_vector->bits[0] ^= data_bits[11];
	error_vector->bits[1] ^= data_bits[12];
	error_vector->bits[2] ^= data_bits[13];
	error_vector->bits[3] ^= data_bits[14];

	return error_vector->bits[0] == 0 &&
		   error_vector->bits[1] == 0 &&
	       error_vector->bits[2] == 0 &&
		   error_vector->bits[3] == 0;
}

static void dmr_bptc_196_96_hamming_13_9_3_get_parity_bits(bool *data_bits, dmr_bptc_196_96_error_vector_t *error_vector) {
	if (data_bits == NULL || error_vector == NULL)
		return;

	error_vector->bits[0] = (data_bits[0] ^ data_bits[1] ^ data_bits[3] ^ data_bits[5] ^ data_bits[6]);
	error_vector->bits[1] = (data_bits[0] ^ data_bits[1] ^ data_bits[2] ^ data_bits[4] ^ data_bits[6] ^ data_bits[7]);
	error_vector->bits[2] = (data_bits[0] ^ data_bits[1] ^ data_bits[2] ^ data_bits[3] ^ data_bits[5] ^ data_bits[7] ^ data_bits[8]);
	error_vector->bits[3] = (data_bits[0] ^ data_bits[2] ^ data_bits[4] ^ data_bits[5] ^ data_bits[8]);
}

// Hamming(13, 9, 3) checking of a matrix column (13 total bits, 9 data bits, min. distance: 3)
static bool dmr_bptc_196_96_hamming_13_9_3_errorcheck(bool *data_bits, dmr_bptc_196_96_error_vector_t *error_vector) {
	if (data_bits == NULL || error_vector == NULL)
		return 0;

	dmr_bptc_196_96_hamming_13_9_3_get_parity_bits(data_bits, error_vector);

	error_vector->bits[0] ^= data_bits[9];
	error_vector->bits[1] ^= data_bits[10];
	error_vector->bits[2] ^= data_bits[11];
	error_vector->bits[3] ^= data_bits[12];

	return error_vector->bits[0] == 0 &&
		   error_vector->bits[1] == 0 &&
		   error_vector->bits[2] == 0 &&
		   error_vector->bits[3] == 0;
}

// Searches for the given error vector in the generator matrix.
// Returns the erroneous bit number if the error vector is found, otherwise it returns -1.
static int dmr_bptc_196_96_find_hamming_15_11_3_error_position(dmr_bptc_196_96_error_vector_t *error_vector) {
	uint8_t row;

	for (row = 0; row < 15; row++) {
		if (hamming_15_11_generator_matrix[row*4] == error_vector->bits[0] &&
			hamming_15_11_generator_matrix[row*4+1] == error_vector->bits[1] &&
			hamming_15_11_generator_matrix[row*4+2] == error_vector->bits[2] &&
			hamming_15_11_generator_matrix[row*4+3] == error_vector->bits[3])
				return row;
	}

	return -1;
}

// Searches for the given error vector in the generator matrix.
// Returns the erroneous bit number if the error vector is found, otherwise it returns -1.
static int dmr_bptc_196_96_find_hamming_13_9_3_error_position(dmr_bptc_196_96_error_vector_t *error_vector) {
	uint8_t row;

	if (error_vector == NULL)
		return -1;

	for (row = 0; row < 13; row++) {
		if (hamming_13_9_generator_matrix[row*4] == error_vector->bits[0] &&
			hamming_13_9_generator_matrix[row*4+1] == error_vector->bits[1] &&
			hamming_13_9_generator_matrix[row*4+2] == error_vector->bits[2] &&
			hamming_13_9_generator_matrix[row*4+3] == error_vector->bits[3])
				return row;
	}

	return -1;
}

/*

// Checks data for errors and tries to repair them.
bool dmr_bptc_196_96_check_and_repair(bit_t deinterleaved_bits[196]) {
	dmr_bptc_196_96_error_vector_t dmr_bptc_196_96_error_vector;
	bit_t column_bits[13] = {0,};
	uint8_t row, col;
	int8_t wrongbitnr = -1;
	bit_t errors_found = 0;
	bool result = true;

	if (deinterleaved_bits == NULL)
		return false;

	for (col = 0; col < 15; col++) {
		for (row = 0; row < 13; row++) {
			// +1 because the first bit is R(3) and it's not used so we can ignore that.
			column_bits[row] = deinterleaved_bits[col+row*15+1];
		}

		if (!dmr_bptc_196_96_hamming_13_9_3_errorcheck(column_bits, &dmr_bptc_196_96_error_vector)) {
			errors_found = 1;
			// Error check failed, checking if we can determine the location of the bit error.
			wrongbitnr = dmr_bptc_196_96_find_hamming_13_9_3_error_position(&dmr_bptc_196_96_error_vector);
			if (wrongbitnr < 0) {
				result = false;
			} else {
				// +1 because the first bit is R(3) and it's not used so we can ignore that.
				deinterleaved_bits[col+wrongbitnr*15+1] = !deinterleaved_bits[col+wrongbitnr*15+1];

				for (row = 0; row < 13; row++) {
					// +1 because the first bit is R(3) and it's not used so we can ignore that.
					column_bits[row] = deinterleaved_bits[col+row*15+1];
				}

				if (!dmr_bptc_196_96_hamming_13_9_3_errorcheck(column_bits, &dmr_bptc_196_96_error_vector)) {
					result = false;
				}
			}
		}
	}

	for (row = 0; row < 9; row++) {
		// +1 because the first bit is R(3) and it's not used so we can ignore that.
		if (!dmr_bptc_196_96_hamming_15_11_3_errorcheck(&deinterleaved_bits[row*15+1], &dmr_bptc_196_96_error_vector)) {
			errors_found = 1;
			// Error check failed, checking if we can determine the location of the bit error.
			wrongbitnr = dmr_bptc_196_96_find_hamming_15_11_3_error_position(&dmr_bptc_196_96_error_vector);
			if (wrongbitnr < 0) {
				result = false;
			} else {
				// +1 because the first bit is R(3) and it's not used so we can ignore that.
				deinterleaved_bits[row*15+wrongbitnr+1] = !deinterleaved_bits[row*15+wrongbitnr+1];

				if (!dmr_bptc_196_96_hamming_15_11_3_errorcheck(&deinterleaved_bits[row*15+1], &dmr_bptc_196_96_error_vector)) {
					result = false;
				}
			}
		}
	}

	if (errors_found && !result) {
		fprintf(stderr, "bptc_196_96: unrecoverable errors found");
	}

	return result;
}

// Generates 196 BPTC payload info bits from 96 data bits.
dmr_fec_payload_bits_t *dmr_bptc_196_96_encode(dmr_bptc_196_96_data_bits_t *data_bits) {
	static dmr_fec_payload_bits_t payload_info_bits;
	dmr_bptc_196_96_error_vector_t error_vector;
	uint8_t col, row;
	uint8_t dbp;
	bit_t column_bits[9] = {0,};

	memset(bptc->raw, 0, sizeof(dmr_fec_payload_bits_t));

	dbp = 0;
	for (row = 0; row < 9; row++) {
		if (row == 0) {
			for (col = 3; col < 11; col++) {
				// +1 because the first bit is R(3) and it's not used so we can ignore that.
				bptc->raw[col+1] = data_bits->bits[dbp++];
			}
		} else {
			for (col = 0; col < 11; col++) {
				// +1 because the first bit is R(3) and it's not used so we can ignore that.
				bptc->raw[col+row*15+1] = data_bits->bits[dbp++];
			}
		}

		// +1 because the first bit is R(3) and it's not used so we can ignore that.
		dmr_bptc_196_96_hamming_15_11_3_get_parity_bits(&bptc->raw[row*15+1], &error_vector);
		bptc->raw[row*15+11+1] = error_vector.bits[0];
		bptc->raw[row*15+12+1] = error_vector.bits[1];
		bptc->raw[row*15+13+1] = error_vector.bits[2];
		bptc->raw[row*15+14+1] = error_vector.bits[3];
	}

	for (col = 0; col < 15; col++) {
		for (row = 0; row < 9; row++)
			column_bits[row] = bptc->raw[col+row*15+1];

		dmr_bptc_196_96_hamming_13_9_3_get_parity_bits(column_bits, &error_vector);
		bptc->raw[col+135+1] = error_vector.bits[0];
		bptc->raw[col+135+15+1] = error_vector.bits[1];
		bptc->raw[col+135+30+1] = error_vector.bits[2];
		bptc->raw[col+135+45+1] = error_vector.bits[3];
	}

	return &payload_info_bits;
}

*/

static int dmr_bptc_196_96_check_and_repair(dmr_bptc_196_96_t *bptc)
{
	if (bptc == NULL)
		return dmr_error(DMR_EINVAL);

	dmr_bptc_196_96_error_vector_t dmr_bptc_196_96_error_vector;
	bool column_bits[13] = {0,};
	uint8_t row, col;
	int8_t wrongbitnr = -1, errors_found = 0;
	bool result = true;

	for (col = 0; col < 15; col++) {
		for (row = 0; row < 13; row++) {
			// +1 because the first bit is R(3) and it's not used so we can ignore that.
			column_bits[row] = bptc->dec[col + row * 15 + 1];
		}

		if (!dmr_bptc_196_96_hamming_13_9_3_errorcheck(column_bits, &dmr_bptc_196_96_error_vector)) {
			errors_found++;
			// Error check failed, checking if we can determine the location of the bit error.
			wrongbitnr = dmr_bptc_196_96_find_hamming_13_9_3_error_position(&dmr_bptc_196_96_error_vector);
			if (wrongbitnr < 0) {
				result = false;
			} else {
				// +1 because the first bit is R(3) and it's not used so we can ignore that.
				bptc->dec[col + wrongbitnr * 15 + 1] = !bptc->dec[col + wrongbitnr * 15 + 1];

				for (row = 0; row < 13; row++) {
					// +1 because the first bit is R(3) and it's not used so we can ignore that.
					column_bits[row] = bptc->dec[col+row*15+1];
				}

				if (!dmr_bptc_196_96_hamming_13_9_3_errorcheck(column_bits, &dmr_bptc_196_96_error_vector)) {
					result = false;
				}
			}
		}
		if (errors_found) {
			if (result)
				dmr_log_debug("bptc_196_96: Hamming(13, 9, 3) check failed in column %d, repaired", col);
			else
				dmr_log_debug("bptc_196_96: Hamming(13, 9, 3) check failed in column %d, not repairable", col);
		}
	}

	for (row = 0; row < 9; row++) {
		// +1 because the first bit is R(3) and it's not used so we can ignore that.
		if (!dmr_bptc_196_96_hamming_15_11_3_errorcheck(&bptc->dec[row * 15 + 1], &dmr_bptc_196_96_error_vector)) {
			errors_found = 1;
			// Error check failed, checking if we can determine the location of the bit error.
			wrongbitnr = dmr_bptc_196_96_find_hamming_15_11_3_error_position(&dmr_bptc_196_96_error_vector);
			if (wrongbitnr < 0) {
				result = false;
			} else {
				// +1 because the first bit is R(3) and it's not used so we can ignore that.
				bptc->dec[row * 15 + wrongbitnr + 1] = !bptc->dec[row * 15 + wrongbitnr + 1];

				if (!dmr_bptc_196_96_hamming_15_11_3_errorcheck(&bptc->dec[row * 15 + 1], &dmr_bptc_196_96_error_vector)) {
					result = false;
				}
			}
		}
		if (errors_found) {
			if (result)
				dmr_log_debug("bptc_196_96: Hamming(15, 11, 3) check failed in row %d, repaired", row);
			else
				dmr_log_debug("bptc_196_96: Hamming(15, 11, 3) check failed in row %d, not repairable", row);
		}
	}

	if (errors_found) {
		if (result)
			dmr_log_debug("bptc_196_96: recovered from %d errors", errors_found);
		else
			dmr_log_error("bptc_196_96: unrecoverable errors found");
	}

	return result;
}

// Extracts the data bits from the given deinterleaved info bits array (discards BPTC bits).
// bptc->dec -> data
static int dmr_bptc_196_96_decode_data(dmr_bptc_196_96_t *bptc, uint8_t *data)
{
	if (bptc == NULL || data == NULL)
		return dmr_error(DMR_EINVAL);

	bool bits[96];

	uint8_t pos = 0, i = 0;
#define C(a, b) do { for (i = a; i < b; i++) bits[pos++] = bptc->dec[i]; } while(0)
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

	dmr_bits_to_bytes(bits, 96, data, 12);
	return 0;
}

static int dmr_bptc_196_96_add_error_bits(dmr_bptc_196_96_t *bptc)
{
	if (bptc == NULL)
		return dmr_error(DMR_EINVAL);

	dmr_bptc_196_96_error_vector_t error_vector;
	uint8_t col, row;
	uint8_t dbp;
	bool column_bits[9] = {0,}, bits[196] = {0, };

	dbp = 0;
	for (row = 0; row < 9; row++) {
		if (row == 0) {
			for (col = 3; col < 11; col++) {
				// +1 because the first bit is R(3) and it's not used so we can ignore that.
				bits[col+1] = bptc->raw[dbp++];
			}
		} else {
			for (col = 0; col < 11; col++) {
				// +1 because the first bit is R(3) and it's not used so we can ignore that.
				bits[col+row*15+1] = bptc->raw[dbp++];
			}
		}

		// +1 because the first bit is R(3) and it's not used so we can ignore that.
		dmr_bptc_196_96_hamming_15_11_3_get_parity_bits(&bits[row*15+1], &error_vector);
		bits[row*15+11+1] = error_vector.bits[0];
		bits[row*15+12+1] = error_vector.bits[1];
		bits[row*15+13+1] = error_vector.bits[2];
		bits[row*15+14+1] = error_vector.bits[3];
	}

	for (col = 0; col < 15; col++) {
		for (row = 0; row < 9; row++)
			column_bits[row] = bits[col+row*15+1];

		dmr_bptc_196_96_hamming_13_9_3_get_parity_bits(column_bits, &error_vector);
		bits[col+135+1] = error_vector.bits[0];
		bits[col+135+15+1] = error_vector.bits[1];
		bits[col+135+30+1] = error_vector.bits[2];
		bits[col+135+45+1] = error_vector.bits[3];
	}

	memcpy(bptc->raw, bits, 196);
	return 0;
}

// data -> bptc->dec
static int dmr_bptc_196_96_encode_data(dmr_bptc_196_96_t *bptc, uint8_t *data)
{
	if (bptc == NULL || data == NULL)
		return dmr_error(DMR_EINVAL);

	bool bits[96];
	dmr_bytes_to_bits(data, 12, bits, 96);
	memset(bptc->dec, 0, sizeof(bptc->dec));

	uint8_t pos = 0, i = 0;
#define C(a, b) do { for (i = a; i < b; i++) bptc->dec[i] = bits[pos++]; } while(0)
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

	return 0;
}

int dmr_bptc_196_96_decode(dmr_bptc_196_96_t *bptc, dmr_packet_t *packet, uint8_t *data)
{
	if (bptc == NULL || packet == NULL || data == NULL)
		return dmr_error(DMR_EINVAL);

	// Decode bytes to bits, packet -> bptc->raw
	dmr_bytes_to_bits(&packet->payload[0] , 12, &bptc->raw[0]  , 96);
	dmr_bytes_to_bits(&packet->payload[21], 12, &bptc->raw[100], 96);
	bool bits[8];
	dmr_byte_to_bits(packet->payload[20], bits);
	bptc->raw[97] = bits[6];
	bptc->raw[98] = bits[7];

	// Deinterleave, bptc->raw -> bptc->dec
	uint8_t i;
	for (i = 0; i < 196; i++)
		bptc->dec[i] = bptc->raw[(i * 181) % 196];

	// Check error bits in bptc->dec
	if (!dmr_bptc_196_96_check_and_repair(bptc))
		return -1;

	// bptc->dec -> data
	return dmr_bptc_196_96_decode_data(bptc, data);
}

int dmr_bptc_196_96_encode(dmr_bptc_196_96_t *bptc, dmr_packet_t *packet, uint8_t *data)
{
	if (bptc == NULL || packet == NULL || data == NULL)
		return dmr_error(DMR_EINVAL);

	// data -> bptc->dec
	int ret;
	if ((ret = dmr_bptc_196_96_encode_data(bptc, data)) != 0)
		return ret;

	// Interleave, bptc->dec -> bptc->raw
	uint8_t i;
	for (i = 0; i < 196; i++)
		bptc->raw[(i * 181) % 196] = bptc->dec[i];

	// Added error bits to bptc->raw
	if ((ret = dmr_bptc_196_96_add_error_bits(bptc)) != 0)
		return ret;

	// Encode bits to bytes, bptc->raw -> packet
	dmr_bits_to_bytes(&bptc->raw[0]  , 96, &packet->payload[0] , 12);
	uint8_t byte = dmr_bits_to_byte(&bptc->raw[96]);
	packet->payload[12] = (packet->payload[12] & 0x3f) | ((byte >> 0) & 0xc0);
	packet->payload[13] = (packet->payload[13] & 0xfc) | ((byte >> 4) & 0x03);
	dmr_bits_to_bytes(&bptc->raw[100], 96, &packet->payload[21], 12);
	return 0;
}
