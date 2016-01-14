#include <talloc.h>
#include <string.h>
#include "dmr/fec/vbptc_16_11.h"
#include "dmr/bits.h"
#include "dmr/error.h"

typedef struct {
    bool bits[5];
} hamming_16_11_error_vector_t;

static bool hamming_16_11_generator_matrix[] = {
	1,	0,	0,	1,	1,
	1,	1,	0,	1,	0,
	1,	1,	1,	1,	1,
	1,	1,	1,	0,	0,
	0,	1,	1,	1,	0,
	1,	0,	1,	0,	1,
	0,	1,	0,	1,	1,
	1,	0,	1,	1,	0,
	1,	1,	0,	0,	1,
	0,	1,	1,	0,	1,
	0,	0,	1,	1,	1,

	1,	0,	0,	0,	0, // These are used to determine errors in the Hamming checksum bits.
	0,	1,	0,	0,	0,
	0,	0,	1,	0,	0,
	0,	0,	0,	1,	0,
	0,	0,	0,	0,	1
};

static void dmr_vbptc_16_11_parity_bits(bool *bits, hamming_16_11_error_vector_t *error_vector)
{
    if (bits == NULL || error_vector == NULL)
        return;

    error_vector->bits[0] = (bits[0] ^ bits[1] ^ bits[2] ^ bits[3] ^ bits[5] ^ bits[7] ^ bits[8]);
    error_vector->bits[1] = (bits[1] ^ bits[2] ^ bits[3] ^ bits[4] ^ bits[6] ^ bits[8] ^ bits[9]);
    error_vector->bits[2] = (bits[2] ^ bits[3] ^ bits[4] ^ bits[5] ^ bits[7] ^ bits[9] ^ bits[10]);
    error_vector->bits[3] = (bits[0] ^ bits[1] ^ bits[2] ^ bits[4] ^ bits[6] ^ bits[7] ^ bits[10]);
    error_vector->bits[4] = (bits[0] ^ bits[2] ^ bits[5] ^ bits[6] ^ bits[8] ^ bits[9] ^ bits[10]);
}

static int8_t dmr_vbptc_16_11_error_position(hamming_16_11_error_vector_t *error_vector)
{
    if (error_vector == NULL)
        return -1;

    uint8_t row;
    for (row = 0; row < 16; row++) {
        if (hamming_16_11_generator_matrix[row * 5 + 0] == error_vector->bits[0] &&
            hamming_16_11_generator_matrix[row * 5 + 1] == error_vector->bits[1] &&
            hamming_16_11_generator_matrix[row * 5 + 2] == error_vector->bits[2] &&
            hamming_16_11_generator_matrix[row * 5 + 3] == error_vector->bits[3] &&
            hamming_16_11_generator_matrix[row * 5 + 4] == error_vector->bits[4]) {
            return row;
        }
    }

    return -1;
}

static bool dmr_vbptc_16_11_check_row(bool *bits, hamming_16_11_error_vector_t *error_vector)
{
    if (bits == NULL || error_vector == NULL)
        return false;

    dmr_vbptc_16_11_parity_bits(bits, error_vector);
    uint8_t offset;
    for (offset = 0; offset < 5; offset++) {
        if ((error_vector->bits[offset] ^ bits[11 + offset]) != 0)
            return false;
    }
    return true;
}

bool dmr_vbptc_16_11_check_and_repair(dmr_vbptc_16_11_t *vbptc)
{
    hamming_16_11_error_vector_t error_vector = { .bits = {0, } };
    uint8_t row, col;
    int8_t error = -1;
    uint8_t errors = 0;
    bool res = true, parity;

    if (vbptc == NULL || vbptc->matrix == NULL || vbptc->rows < 2)
        return 0;

    for (row = 0; row < vbptc->rows - 1; row++) {
        if (!dmr_vbptc_16_11_check_row(vbptc->matrix + row * 16, &error_vector)) {
            errors++;
            error = dmr_vbptc_16_11_error_position(&error_vector);
            if (error < 0) {
                dmr_log_error("vbptc_16_11: Hamming(16, 11) error, can't repair row #%u", row);
                res = false;
            } else {
                dmr_log_debug("vbptc_16_11: Hamming(16, 11) error, fixing bit %u in row #%u", row, error);
                vbptc->matrix[row * 16 + error] = !vbptc->matrix[row * 16 + error];
                if (!dmr_vbptc_16_11_check_row(vbptc->matrix + row * 16, &error_vector)) {
                    dmr_log_error("vbptc_16_11: Hamming(16, 11) error, repair failed on row #%u", row);
                    res = false;
                }
            }
        }
    }

    for (col = 0; col < 16; col++) {
        parity = false;
        for (row = 0; row < vbptc->rows - 1; row++)
            parity = (parity + vbptc->matrix[row * 16 + col]) % 2;

        if (parity != vbptc->matrix[(vbptc->rows - 1) * 16 + col]) {
            dmr_log_error("vbptc_16_11: parity check error in col #%u", col);
            return false;
        }
    }

    if (res && !errors) {
        dmr_log_debug("vbptc_16_11: received data has no errors");
    } else if (res && errors) {
        dmr_log_debug("vbptc_16_11: received data has %d recoverable errors", errors);
    } else if (!res) {
        dmr_log_error("vbptc_16_11: received data has errors that can't be corrected");
    }

    return res;
}

dmr_vbptc_16_11_t *dmr_vbptc_16_11_new(uint8_t rows, void *parent)
{
    dmr_vbptc_16_11_t *vbptc = talloc_zero(parent, dmr_vbptc_16_11_t);
    if (vbptc == NULL)
        return NULL;

    vbptc->matrix = (bool *)talloc_size(vbptc, sizeof(bool) * rows * 16);
    if (vbptc->matrix == NULL) {
        dmr_log_error("vbptc_16_11: error allocating space for matrix");
        dmr_error(DMR_ENOMEM);
        return NULL;
    }
    vbptc->rows = rows;
    return vbptc;
}

void dmr_vbptc_16_11_free(dmr_vbptc_16_11_t *vbptc)
{
    if (vbptc == NULL)
        return;

    talloc_free(vbptc);
}

void dmr_vbptc_16_11_wipe(dmr_vbptc_16_11_t *vbptc)
{
    if (vbptc == NULL)
        return;

    vbptc->row = 0;
    vbptc->col = 0;
    if (vbptc->matrix != NULL)
        memset(vbptc->matrix, 0, sizeof(bool) * vbptc->rows * 16);
}

static size_t dmr_vbptc_16_11_matrix_free(dmr_vbptc_16_11_t *vbptc)
{
    if (vbptc == NULL)
        return 0;

    return (vbptc->rows * 16) - (vbptc->col * vbptc->rows + vbptc->row);
}

int dmr_vbptc_16_11_add(dmr_vbptc_16_11_t *vbptc, bool *bits, size_t len)
{
    if (vbptc == NULL || vbptc->matrix == NULL || bits == NULL)
        return dmr_error(DMR_EINVAL);

    size_t space = dmr_vbptc_16_11_matrix_free(vbptc);
    if (space == 0) {
        dmr_log_error("vbptc_16_11: can't add burst, no free space in matrix");
        return -1;
    }

    uint8_t bitlen = min(len, space), i;
    for (i = 0; i < bitlen; i++) {
        vbptc->matrix[vbptc->col + vbptc->row * 16] = bits[i];
        if (++vbptc->row == vbptc->rows) {
            vbptc->col++;
            vbptc->row = 0;
        }
    }

    return 0;
}

int dmr_vbptc_16_11_get_fragment(dmr_vbptc_16_11_t *vbptc, bool *bits, uint16_t offset, uint16_t len)
{
    if (vbptc == NULL || vbptc->matrix == NULL || bits == NULL)
        return dmr_error(DMR_EINVAL);

    uint8_t col, row;
    uint16_t bitlen = min(vbptc->rows * 16, len), pos = 0;
    for (col = 0; col < 16; col++) {
        for (row = 0; row < vbptc->rows; row++) {
            if (pos >= bitlen)
                break;

            if (offset == 0) {
                bits[pos++] = vbptc->matrix[row * 16 + col];
            } else {
                offset--;
            }
        }
    }

    return 0;
}

int dmr_vbptc_16_11_decode(dmr_vbptc_16_11_t *vbptc, bool *bits, size_t len)
{
    if (vbptc == NULL || bits == NULL || vbptc->rows == 0)
        return dmr_error(DMR_EINVAL);

    uint8_t row, col;
    for (row = 0; row < vbptc->rows - 1; row++) {
        for (col = 0; col < 11; col++) {
            if (row * 11 + col >= len)
                break;

            bits[row * 11 + col] = vbptc->matrix[row * 16 + col];
        }
    }

    return 0;
}

int dmr_vbptc_16_11_encode(dmr_vbptc_16_11_t *vbptc, bool *bits, size_t len)
{
    if (vbptc == NULL || bits == NULL || len == 0)
        return dmr_error(DMR_EINVAL);

    dmr_vbptc_16_11_wipe(vbptc);
    size_t bitlen = min(len, dmr_vbptc_16_11_matrix_free(vbptc));
    uint8_t col, row;
    hamming_16_11_error_vector_t parity_bits;

    for (col = 0; col < bitlen; col++) {
        vbptc->matrix[vbptc->col + vbptc->row * 16] = bits[col];
        if (++vbptc->col == 11) {
            vbptc->row++;
            vbptc->col = 0;
        }
    }

    // Calculating Hamming(16, 11) parities
    for (row = 0; row < vbptc->rows - 1; row++) {
        dmr_vbptc_16_11_parity_bits(&vbptc->matrix[row * 16], &parity_bits);
        memcpy(&vbptc->matrix[row * 16 + 11], parity_bits.bits, sizeof(hamming_16_11_error_vector_t));
    }

    for (col = 0; col < 16; col++) {
        bool parity = false;
        for (row = 0; row < vbptc->rows - 1; row++)
            parity = parity + vbptc->matrix[row * 16 + col] % 2;

        vbptc->matrix[(vbptc->rows - 1) * 16 + col] = parity;
    }

    return 0;
}
