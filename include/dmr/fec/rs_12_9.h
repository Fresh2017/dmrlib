/**
 * @file
 * @brief  ...
 * @author Wijnand Modderman-Lenstra PD0MZ
 */
#ifndef _DMR_FEC_RS_12_9_H
#define _DMR_FEC_RS_12_9_H

#ifdef __cplusplus
extern "C" {
#endif

#include <inttypes.h>

extern int dmr_rs_12_9_4_decode(uint8_t bytes[12], uint8_t crc_mask);
extern int dmr_rs_12_9_4_encode(uint8_t bytes[12], uint8_t crc_mask);

#ifdef __cplusplus
}
#endif

#endif // _DMR_FEC_RS_12_9_H
