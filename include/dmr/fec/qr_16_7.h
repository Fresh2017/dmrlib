#ifndef _DMR_FEC_QR_16_7
#define _DMR_FEC_QR_16_7

#include <inttypes.h>
#include <stdbool.h>

void dmr_qr_16_7_encode(uint8_t buf[2]);
bool dmr_qr_16_7_decode(uint8_t buf[2]);

#endif // _DMR_FEC_QR_16_7
