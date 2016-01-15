#ifndef _DMR_BITS_H
#define _DMR_BITS_H

#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <dmr/bits/b.h>

#define DMR_PAYLOAD_BYTES           33
#define DMR_PAYLOAD_BITS            (DMR_PAYLOAD_BYTES*8)

#define DMR_PAYLOAD_SLOT_TYPE_HALF  10
#define DMR_PAYLOAD_SLOT_TYPE_BITS  (DMR_PAYLOAD_SLOT_TYPE_HALF*2)

#define HEXDUMP_COLS                16

#ifndef min
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif
#ifndef max
#define max(a,b) (((a) > (b)) ? (a) : (b))
#endif

typedef uint8_t dmr_bit_t;                  // 1 bits per byte
typedef uint8_t dmr_dibit_t;                // 2 bits per byte
typedef uint8_t dmr_tribit_t;               // 3 bits per byte

#define DMR_UINT16_LE(b0, b1)         (uint16_t)((uint8_t)(b0) | ((uint8_t)(b1) << 8))
#define DMR_UINT16_BE(b0, b1)         (uint16_t)(((uint8_t)(b0) << 8) | (uint8_t)(b1))
#define DMR_UINT16_LE_PACK(b, n)      do { *b = n; *(b+1) = (n << 8); } while(0)
#define DMR_UINT16_BE_PACK(b, n)      do { *b = (n << 8); *(b+1) = n; } while(0)
#define DMR_UINT32_LE(b0, b1, b2, b3) (uint32_t)((uint8_t)(b0) | ((uint8_t)(b1) << 8) | ((uint8_t)(b2) << 16) | ((uint8_t)(b3) << 24))
#define DMR_UINT32_BE(b0, b1, b2, b3) (uint32_t)(((uint8_t)(b0) << 24) | ((uint8_t)(b1) << 16) | ((uint8_t)(b2) << 8) | (uint8_t)(b3))
#define DMR_UINT32_BE_UNPACK(b)       (uint32_t)((*(b) << 24) | (*(b+1) << 16) | (*(b+2) << 8) | (*(b+3)))
#define DMR_UINT32_BE_UNPACK3(b)      (uint32_t)(               (*(b+1) << 16) | (*(b+2) << 8) | (*(b+3)))
#define DMR_UINT32_LE_PACK(b, n)      do { *(b) = (n <<  0); *(b+1) = (n <<  8); *(b+2) = (n << 16); *(b+3) = (n << 24); } while(0)
#define DMR_UINT32_LE_PACK3(b, n)     do { *(b) = (n <<  0); *(b+1) = (n <<  8); *(b+2) = (n << 16);                     } while(0)
#define DMR_UINT32_BE_PACK(b, n)      do { *(b) = (n << 24); *(b+1) = (n << 16); *(b+2) = (n <<  8); *(b+3) = (n <<  0); } while(0)
#define DMR_UINT32_BE_PACK3(b, n)     do { 					 *(b+1) = (n << 16); *(b+2) = (n <<  8); *(b+3) = (n <<  0); } while(0)

extern char *dmr_byte_to_binary(uint8_t byte);
extern uint8_t dmr_bits_to_byte(bool bits[8]);
extern void dmr_bits_to_bytes(bool *bits, size_t bits_length, uint8_t *bytes, size_t bytes_length);
extern void dmr_byte_to_bits(uint8_t byte, bool bits[8]);
extern void dmr_bytes_to_bits(uint8_t *bytes, size_t bytes_length, bool *bits, size_t bits_length);
void _dmr_dump_hex(void *mem, size_t len, const char *func, size_t line);

#define dump_hex(mem, len)      _dmr_dump_hex(mem, len, __FILE__, __LINE__)

#endif // _DMR_BITS_H
