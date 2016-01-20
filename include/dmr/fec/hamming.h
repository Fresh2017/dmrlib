#ifndef _DMR_FEC_HAMMING_H
#define _DMR_FEC_HAMMING_H

#ifdef __cplusplus
extern "C" {
#endif

#include <inttypes.h>
#include <stdbool.h>

extern void dmr_hamming_7_4_3_encode(bool bits[7]);
extern bool dmr_hamming_7_4_3_decode(bool bits[7]);
extern void dmr_hamming_13_9_3_encode(bool bits[13]);
extern bool dmr_hamming_13_9_3_decode(bool bits[13]);
extern void dmr_hamming_15_11_3_encode(bool bits[15]);
extern bool dmr_hamming_15_11_3_decode(bool bits[15]);
extern void dmr_hamming_16_11_4_encode(bool bits[16]);
extern bool dmr_hamming_16_11_4_decode(bool bits[16]);
extern void dmr_hamming_17_12_3_encode(bool bits[17]);
extern bool dmr_hamming_17_12_3_decode(bool bits[17]);

#ifdef __cplusplus
}
#endif

#endif // _DMR_FEC_HAMMING_H
