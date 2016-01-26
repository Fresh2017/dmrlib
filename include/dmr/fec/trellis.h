/**
 * @file
 * @brief  Trellis encoding
 * @author Wijnand Modderman-Lenstra PD0MZ
 */
#ifndef _DMR_FEC_TRELLIS_H
#define _DMR_FEC_TRELLIS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <inttypes.h>
#include <dmr/packet.h>

extern int dmr_trellis_rate_34_decode(uint8_t bytes[18], dmr_packet_t *packet);

#ifdef __cplusplus
}
#endif

#endif // _DMR_FEC_TRELLIS_H
