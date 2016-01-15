#ifndef _DMR_FEC_HAMMING_H
#define _DMR_FEC_HAMMING_H

#include <stdbool.h>

extern void dmr_hamming_13_9_3_encode_bits(bool bits[13]);
extern void dmr_hamming_15_11_3_encode_bits(bool bits[15]);

#endif // _DMR_FEC_HAMMING_H