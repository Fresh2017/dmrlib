#ifndef _DMR_BITS_H
#define _DMR_BITS_H

#include <inttypes.h>
#include <stddef.h>

#define HEXDUMP_COLS 16

#ifndef min
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif
#ifndef max
#define max(a,b) (((a) > (b)) ? (a) : (b))
#endif

typedef uint8_t bit_t;                  // 1 bits per byte
typedef uint8_t dibit_t;                // 2 bits per byte
typedef uint8_t tribit_t;               // 3 bits per byte

uint8_t bits_to_byte(bit_t bits[8]);
void bits_to_bytes(bit_t *bits, size_t bits_length, uint8_t *bytes, size_t bytes_length);
void byte_to_bits(uint8_t byte, bit_t bits[8]);
void bytes_to_bits(uint8_t *bytes, size_t bytes_length, bit_t *bits, size_t bits_length);
void _dmr_dump_hex(void *mem, size_t len, const char *func, size_t line);

#define dump_hex(mem, len)      _dmr_dump_hex(mem, len, __FILE__, __LINE__)

#endif // _DMR_BITS_H
