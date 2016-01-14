#ifndef _DMR_FEC_RS_12_9_H
#define _DMR_FEC_RS_12_9_H

#include <dmr/bits.h>

#define DMR_RS_12_9_DATASIZE		9
#define DMR_RS_12_9_CHECKSUMSIZE	3

typedef struct {
	uint8_t data[DMR_RS_12_9_DATASIZE+DMR_RS_12_9_CHECKSUMSIZE];
} dmr_rs_12_9_codeword_t;

// Maximum degree of various polynomials.
#define DMR_RS_12_9_POLY_MAXDEG (DMR_RS_12_9_CHECKSUMSIZE*2)

typedef struct {
	uint8_t data[DMR_RS_12_9_POLY_MAXDEG];
} dmr_rs_12_9_poly_t;

#define DMR_RS_12_9_CORRECT_ERRORS_RESULT_NO_ERRORS_FOUND			0
#define DMR_RS_12_9_CORRECT_ERRORS_RESULT_ERRORS_CORRECTED			1
#define DMR_RS_12_9_CORRECT_ERRORS_RESULT_ERRORS_CANT_BE_CORRECTED	2
typedef uint8_t dmr_rs_12_9_correct_errors_result_t;

typedef struct {
	uint8_t bytes[3];
} dmr_rs_12_9_checksum_t;

extern void dmr_rs_12_9_calc_syndrome(dmr_rs_12_9_codeword_t *codeword, dmr_rs_12_9_poly_t *syndrome);
extern bool dmr_rs_12_9_check_syndrome(dmr_rs_12_9_poly_t *syndrome);
extern dmr_rs_12_9_correct_errors_result_t dmr_rs_12_9_correct_errors(dmr_rs_12_9_codeword_t *codeword, dmr_rs_12_9_poly_t *syndrome, uint8_t *errors_found);
extern dmr_rs_12_9_checksum_t *dmr_rs_12_9_calc_checksum(dmr_rs_12_9_codeword_t *codeword);

#endif // _DMR_FEC_RS_12_9_H
