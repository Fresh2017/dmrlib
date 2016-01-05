#include <string.h>

#include "dmrfec/bptc_196_96.h"

typedef struct {
	bit_t bits[4];
} dmrfec_bptc_196_96_error_vector_t;

// Hamming(15, 11, 3) checking of a matrix row (15 total bits, 11 data bits, min. distance: 3)
// See page 135 of the DMR Air Interface protocol specification for the generator matrix.
// A generator matrix looks like this: G = [Ik | P]. The parity check matrix is: H = [-P^T|In-k]
// In binary codes, then -P = P, so the negation is unnecessary. We can get the parity check matrix
// only by transposing the generator matrix. We then take a data row, and multiply it with each row
// of the parity check matrix, then xor each resulting row bits together with the corresponding
// parity check bit. The xor result (error vector) should be 0, if it's not, it can be used
// to determine the location of the erroneous bit using the generator matrix (P).
static void dmrfec_bptc_196_96_hamming_15_11_3_get_parity_bits(bit_t *data_bits, dmrfec_bptc_196_96_error_vector_t *error_vector) {
	if (data_bits == NULL || error_vector == NULL)
		return;

	error_vector->bits[0] = (data_bits[0] ^ data_bits[1] ^ data_bits[2] ^ data_bits[3] ^ data_bits[5] ^ data_bits[7] ^ data_bits[8]);
	error_vector->bits[1] = (data_bits[1] ^ data_bits[2] ^ data_bits[3] ^ data_bits[4] ^ data_bits[6] ^ data_bits[8] ^ data_bits[9]);
	error_vector->bits[2] = (data_bits[2] ^ data_bits[3] ^ data_bits[4] ^ data_bits[5] ^ data_bits[7] ^ data_bits[9] ^ data_bits[10]);
	error_vector->bits[3] = (data_bits[0] ^ data_bits[1] ^ data_bits[2] ^ data_bits[4] ^ data_bits[6] ^ data_bits[7] ^ data_bits[10]);
}

static bool dmrfec_bptc_196_96_hamming_15_11_3_errorcheck(bit_t *data_bits, dmrfec_bptc_196_96_error_vector_t *error_vector) {
	if (data_bits == NULL || error_vector == NULL)
		return 0;

	dmrfec_bptc_196_96_hamming_15_11_3_get_parity_bits(data_bits, error_vector);

	error_vector->bits[0] ^= data_bits[11];
	error_vector->bits[1] ^= data_bits[12];
	error_vector->bits[2] ^= data_bits[13];
	error_vector->bits[3] ^= data_bits[14];

	return error_vector->bits[0] == 0 &&
		   error_vector->bits[1] == 0 &&
	       error_vector->bits[2] == 0 &&
		   error_vector->bits[3] == 0;
}

static void dmrfec_bptc_196_96_hamming_13_9_3_get_parity_bits(bit_t *data_bits, dmrfec_bptc_196_96_error_vector_t *error_vector) {
	if (data_bits == NULL || error_vector == NULL)
		return;

	error_vector->bits[0] = (data_bits[0] ^ data_bits[1] ^ data_bits[3] ^ data_bits[5] ^ data_bits[6]);
	error_vector->bits[1] = (data_bits[0] ^ data_bits[1] ^ data_bits[2] ^ data_bits[4] ^ data_bits[6] ^ data_bits[7]);
	error_vector->bits[2] = (data_bits[0] ^ data_bits[1] ^ data_bits[2] ^ data_bits[3] ^ data_bits[5] ^ data_bits[7] ^ data_bits[8]);
	error_vector->bits[3] = (data_bits[0] ^ data_bits[2] ^ data_bits[4] ^ data_bits[5] ^ data_bits[8]);
}

// Hamming(13, 9, 3) checking of a matrix column (13 total bits, 9 data bits, min. distance: 3)
static bool dmrfec_bptc_196_96_hamming_13_9_3_errorcheck(bit_t *data_bits, dmrfec_bptc_196_96_error_vector_t *error_vector) {
	if (data_bits == NULL || error_vector == NULL)
		return 0;

	dmrfec_bptc_196_96_hamming_13_9_3_get_parity_bits(data_bits, error_vector);

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
static int dmrfec_bptc_196_96_find_hamming_15_11_3_error_position(dmrfec_bptc_196_96_error_vector_t *error_vector) {
	static bit_t hamming_15_11_generator_matrix[] = {
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
static int dmrfec_bptc_196_96_find_hamming_13_9_3_error_position(dmrfec_bptc_196_96_error_vector_t *error_vector) {
	static bit_t hamming_13_9_generator_matrix[] = {
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

// Checks data for errors and tries to repair them.
bool dmrfec_bptc_196_96_check_and_repair(bit_t deinterleaved_bits[196]) {
	dmrfec_bptc_196_96_error_vector_t dmrfec_bptc_196_96_error_vector;
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

		if (!dmrfec_bptc_196_96_hamming_13_9_3_errorcheck(column_bits, &dmrfec_bptc_196_96_error_vector)) {
			errors_found = 1;
			// Error check failed, checking if we can determine the location of the bit error.
			wrongbitnr = dmrfec_bptc_196_96_find_hamming_13_9_3_error_position(&dmrfec_bptc_196_96_error_vector);
			if (wrongbitnr < 0) {
				result = false;
			} else {
				// +1 because the first bit is R(3) and it's not used so we can ignore that.
				deinterleaved_bits[col+wrongbitnr*15+1] = !deinterleaved_bits[col+wrongbitnr*15+1];

				for (row = 0; row < 13; row++) {
					// +1 because the first bit is R(3) and it's not used so we can ignore that.
					column_bits[row] = deinterleaved_bits[col+row*15+1];
				}

				if (!dmrfec_bptc_196_96_hamming_13_9_3_errorcheck(column_bits, &dmrfec_bptc_196_96_error_vector)) {
					result = false;
				}
			}
		}
	}

	for (row = 0; row < 9; row++) {
		// +1 because the first bit is R(3) and it's not used so we can ignore that.
		if (!dmrfec_bptc_196_96_hamming_15_11_3_errorcheck(&deinterleaved_bits[row*15+1], &dmrfec_bptc_196_96_error_vector)) {
			errors_found = 1;
			// Error check failed, checking if we can determine the location of the bit error.
			wrongbitnr = dmrfec_bptc_196_96_find_hamming_15_11_3_error_position(&dmrfec_bptc_196_96_error_vector);
			if (wrongbitnr < 0) {
				result = false;
			} else {
				// +1 because the first bit is R(3) and it's not used so we can ignore that.
				deinterleaved_bits[row*15+wrongbitnr+1] = !deinterleaved_bits[row*15+wrongbitnr+1];

				if (!dmrfec_bptc_196_96_hamming_15_11_3_errorcheck(&deinterleaved_bits[row*15+1], &dmrfec_bptc_196_96_error_vector)) {
					result = false;
				}
			}
		}
	}

	return result;
}

// Extracts the data bits from the given deinterleaved info bits array (discards BPTC bits).
dmrfec_bptc_196_96_data_bits_t *dmrfec_bptc_196_96_extractdata(bit_t deinterleaved_bits[196]) {
	static dmrfec_bptc_196_96_data_bits_t data_bits;

	if (deinterleaved_bits == NULL)
		return NULL;

	memcpy(&data_bits.bits[0], &deinterleaved_bits[4], 8);
	memcpy(&data_bits.bits[8], &deinterleaved_bits[16], 11);
	memcpy(&data_bits.bits[19], &deinterleaved_bits[31], 11);
	memcpy(&data_bits.bits[30], &deinterleaved_bits[46], 11);
	memcpy(&data_bits.bits[41], &deinterleaved_bits[61], 11);
	memcpy(&data_bits.bits[52], &deinterleaved_bits[76], 11);
	memcpy(&data_bits.bits[63], &deinterleaved_bits[91], 11);
	memcpy(&data_bits.bits[74], &deinterleaved_bits[106], 11);
	memcpy(&data_bits.bits[85], &deinterleaved_bits[121], 11);

	return &data_bits;
}

/*
// Generates 196 BPTC payload info bits from 96 data bits.
dmrpacket_payload_info_bits_t *dmrfec_bptc_196_96_generate(dmrfec_bptc_196_96_data_bits_t *data_bits) {
	static dmrpacket_payload_info_bits_t payload_info_bits;
	dmrfec_bptc_196_96_error_vector_t error_vector;
	uint8_t col, row;
	uint8_t dbp;
	bit_t column_bits[9] = {0,};

	memset(payload_info_bits.bits, 0, sizeof(dmrpacket_payload_info_bits_t));

	dbp = 0;
	for (row = 0; row < 9; row++) {
		if (row == 0) {
			for (col = 3; col < 11; col++) {
				// +1 because the first bit is R(3) and it's not used so we can ignore that.
				payload_info_bits.bits[col+1] = data_bits->bits[dbp++];
			}
		} else {
			for (col = 0; col < 11; col++) {
				// +1 because the first bit is R(3) and it's not used so we can ignore that.
				payload_info_bits.bits[col+row*15+1] = data_bits->bits[dbp++];
			}
		}

		// +1 because the first bit is R(3) and it's not used so we can ignore that.
		dmrfec_bptc_196_96_hamming_15_11_3_get_parity_bits(&payload_info_bits.bits[row*15+1], &error_vector);
		payload_info_bits.bits[row*15+11+1] = error_vector.bits[0];
		payload_info_bits.bits[row*15+12+1] = error_vector.bits[1];
		payload_info_bits.bits[row*15+13+1] = error_vector.bits[2];
		payload_info_bits.bits[row*15+14+1] = error_vector.bits[3];
	}

	for (col = 0; col < 15; col++) {
		for (row = 0; row < 9; row++)
			column_bits[row] = payload_info_bits.bits[col+row*15+1];

		dmrfec_bptc_196_96_hamming_13_9_3_get_parity_bits(column_bits, &error_vector);
		payload_info_bits.bits[col+135+1] = error_vector.bits[0];
		payload_info_bits.bits[col+135+15+1] = error_vector.bits[1];
		payload_info_bits.bits[col+135+30+1] = error_vector.bits[2];
		payload_info_bits.bits[col+135+45+1] = error_vector.bits[3];
	}

	return &payload_info_bits;
}
*/
