#ifndef _DMRFEC_RS_12_9_H
#define _DMRFEC_RS_12_9_H

#include <dmr/bits.h>

#define DMRFEC_RS_12_9_DATASIZE		9
#define DMRFEC_RS_12_9_CHECKSUMSIZE	3

typedef struct {
	uint8_t data[DMRFEC_RS_12_9_DATASIZE+DMRFEC_RS_12_9_CHECKSUMSIZE];
} dmrfec_rs_12_9_codeword_t;

// Maximum degree of various polynomials.
#define DMRFEC_RS_12_9_POLY_MAXDEG (DMRFEC_RS_12_9_CHECKSUMSIZE*2)

typedef struct {
	uint8_t data[DMRFEC_RS_12_9_POLY_MAXDEG];
} dmrfec_rs_12_9_poly_t;

#define DMRFEC_RS_12_9_CORRECT_ERRORS_RESULT_NO_ERRORS_FOUND			0
#define DMRFEC_RS_12_9_CORRECT_ERRORS_RESULT_ERRORS_CORRECTED			1
#define DMRFEC_RS_12_9_CORRECT_ERRORS_RESULT_ERRORS_CANT_BE_CORRECTED	2
typedef uint8_t dmrfec_rs_12_9_correct_errors_result_t;

typedef struct {
	uint8_t bytes[3];
} dmrfec_rs_12_9_checksum_t;

void dmrfec_rs_12_9_calc_syndrome(dmrfec_rs_12_9_codeword_t *codeword, dmrfec_rs_12_9_poly_t *syndrome);
bit_t dmrfec_rs_12_9_check_syndrome(dmrfec_rs_12_9_poly_t *syndrome);
dmrfec_rs_12_9_correct_errors_result_t dmrfec_rs_12_9_correct_errors(dmrfec_rs_12_9_codeword_t *codeword, dmrfec_rs_12_9_poly_t *syndrome, uint8_t *errors_found);
dmrfec_rs_12_9_checksum_t *dmrfec_rs_12_9_calc_checksum(dmrfec_rs_12_9_codeword_t *codeword);

#endif // _DMRFEC_RS_12_9_H
