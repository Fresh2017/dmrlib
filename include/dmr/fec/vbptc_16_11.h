#ifndef _DMR_FEC_VBPTC_16_11_H
#define _DMR_FEC_VBPTC_16_11_H

#include <inttypes.h>
#include <stdbool.h>

typedef struct {
	bool    *matrix;
	uint8_t row;
	uint8_t col;
	uint8_t rows;
} dmr_vbptc_16_11_t;

extern bool dmr_vbptc_16_11_check_and_repair(dmr_vbptc_16_11_t *vbptc);
extern dmr_vbptc_16_11_t *dmr_vbptc_16_11_new(uint8_t rows, void *parent);
extern void dmr_vbptc_16_11_free(dmr_vbptc_16_11_t *);
extern void dmr_vbptc_16_11_wipe(dmr_vbptc_16_11_t *vbptc);
extern int dmr_vbptc_16_11_add(dmr_vbptc_16_11_t *vbptc, bool *bits, size_t len);
extern int dmr_vbptc_16_11_get_fragment(dmr_vbptc_16_11_t *vbptc, bool *bits, uint16_t offset, uint16_t len);
extern int dmr_vbptc_16_11_decode(dmr_vbptc_16_11_t *vbptc, bool *bits, size_t len);
extern int dmr_vbptc_16_11_encode(dmr_vbptc_16_11_t *vbptc, bool *bits, size_t len);

#endif // _DMR_FEC_VBPTC_16_11_H
