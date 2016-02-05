#ifndef _DMR_FEC_BPTC_196_96_H
#define _DMR_FEC_BPTC_196_96_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <dmr/bits.h>
#include <dmr/packet.h>

typedef struct {
	bool raw[196];
	bool deinterleaved_bits[196];
} dmr_bptc_196_96;

int dmr_bptc_196_96_decode(dmr_packet packet, dmr_bptc_196_96 *bptc, uint8_t data[12]);
int dmr_bptc_196_96_encode(dmr_packet packet, dmr_bptc_196_96 *bptc, uint8_t data[12]);

#ifdef __cplusplus
}
#endif

#endif // _DMR_FEC_BPTC_196_96_H
