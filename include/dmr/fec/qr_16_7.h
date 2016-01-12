#ifndef _DMR_FEC_QR_16_7
#define _DMR_FEC_QR_16_7

#include <inttypes.h>

void dmr_fec_qr_16_7_encode(uint8_t buf[2]);
void dmr_fec_qr_16_7_decode(uint8_t buf[2], uint8_t *byte);

#endif // _DMR_FEC_QR_16_7
