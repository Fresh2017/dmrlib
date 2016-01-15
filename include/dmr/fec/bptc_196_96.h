#ifndef _DMR_FEC_BPTC_196_96_H
#define _DMR_FEC_BPTC_196_96_H

#include <stdbool.h>
#include <dmr/bits.h>
#include <dmr/packet.h>

typedef struct {
	bool raw[196];
	bool deinterleaved_bits[196];
} dmr_bptc_196_96_t;

int dmr_bptc_196_96_decode(dmr_bptc_196_96_t *bptc, dmr_packet_t *packet, uint8_t *data);
int dmr_bptc_196_96_encode(dmr_bptc_196_96_t *bptc, dmr_packet_t *packet, uint8_t *data);

#endif // _DMR_FEC_BPTC_196_96_H
