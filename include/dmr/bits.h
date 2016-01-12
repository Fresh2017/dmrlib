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

extern char *dmr_byte_to_binary(uint8_t byte);
extern uint8_t dmr_bits_to_byte(bool bits[8]);
extern void dmr_bits_to_bytes(bool *bits, size_t bits_length, uint8_t *bytes, size_t bytes_length);
extern void dmr_byte_to_bits(uint8_t byte, bool bits[8]);
extern void dmr_bytes_to_bits(uint8_t *bytes, size_t bytes_length, bool *bits, size_t bits_length);
void _dmr_dump_hex(void *mem, size_t len, const char *func, size_t line);

#define dump_hex(mem, len)      _dmr_dump_hex(mem, len, __FILE__, __LINE__)

#endif // _DMR_BITS_H
