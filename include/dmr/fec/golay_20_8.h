#ifndef _DMR_FEC_GOLAY_20_8_H
#define _DMR_FEC_GOLAY_20_8_H

#ifdef __cplusplus
extern "C" {
#endif

#include <inttypes.h>

extern uint8_t dmr_golay_20_8_decode(uint8_t data[3]);
extern void dmr_golay_20_8_encode(uint8_t data[3]);

#ifdef __cplusplus
}
#endif

#endif // _DMR_FEC_GOLAY_20_8_H
