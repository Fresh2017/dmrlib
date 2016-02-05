#ifndef _COMMON_UINT_H
#define _COMMON_UINT_H

#include <inttypes.h>
#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __uint24_t
#define __uint24_t __uint32_t
#define uint24_t   uint32_t
#endif // __uint24_t

uint16_t   uint16(uint8_t *buf);
uint16_t   uint16_le(uint8_t *buf);
uint24_t   uint24(uint8_t *buf);
uint24_t   uint24_le(uint8_t *buf);
uint32_t   uint32(uint8_t *buf);
uint32_t   uint32_le(uint8_t *buf);
uint64_t   uint64(uint8_t *buf);
uint64_t   uint64_le(uint8_t *buf);
void       uint16_pack(uint8_t *buf, uint16_t in);
void       uint16_pack_le(uint8_t *buf, uint16_t in);
void       uint24_pack(uint8_t *buf, uint24_t in);
void       uint24_pack_le(uint8_t *buf, uint24_t in);
void       uint32_pack(uint8_t *buf, uint32_t in);
void       uint32_pack_le(uint8_t *buf, uint32_t in);
void       uint64_pack(uint8_t *buf, uint64_t in);
void       uint64_pack_le(uint8_t *buf, uint64_t in);
const char xuint[16];
void       xuint8_pack(uint8_t *buf, uint8_t in);
#define    xuint8_pack_le xuint8_pack
void       xuint16_pack(uint8_t *buf, uint16_t in);
void       xuint16_pack_le(uint8_t *buf, uint16_t in);
void       xuint24_pack(uint8_t *buf, uint24_t in);
void       xuint24_pack_le(uint8_t *buf, uint24_t in);
void       xuint32_pack(uint8_t *buf, uint32_t in);
void       xuint32_pack_le(uint8_t *buf, uint32_t in);
void       xuint64_pack(uint8_t *buf, uint64_t in);
void       xuint64_pack_le(uint8_t *buf, uint64_t in);

#ifdef __cplusplus
}
#endif

#endif // _COMMON_UINT_H
