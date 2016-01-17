#ifndef _DMR_FEC_RS_12_9_H
#define _DMR_FEC_RS_12_9_H

#ifdef __cplusplus
extern "C" {
#endif

#include <dmr/bits.h>

#define DMR_RS_12_9_DATASIZE		9
#define DMR_RS_12_9_CHECKSUMSIZE	3

extern void dmr_rs_12_9_encode(uint8_t data[DMR_RS_12_9_DATASIZE], uint8_t parity[DMR_RS_12_9_CHECKSUMSIZE]);
extern bool dmr_rs_12_9_check(uint8_t data[DMR_RS_12_9_DATASIZE + DMR_RS_12_9_CHECKSUMSIZE]);

#ifdef __cplusplus
}
#endif

#endif // _DMR_FEC_RS_12_9_H
