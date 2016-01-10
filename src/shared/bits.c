#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include "dmr/bits.h"
#include "dmr/log.h"

uint8_t bits_to_byte(bit_t bits[8])
{
    uint8_t val = 0, i = 0;
    for (; i < 8; i++) {
        if (bits[i])
            val |= (1 << (7 - i));
    }
    return val;
}

void bits_to_bytes(bit_t *bits, size_t bits_length, uint8_t *bytes, size_t bytes_length)
{
    uint16_t i = 0;
    for (; i < min(bits_length/8, bytes_length); i++)
        bytes[i] = bits_to_byte(&bits[i * 8]);
}

void byte_to_bits(uint8_t byte, bit_t bits[8]) {
	  bits[0] = (byte & 128) > 0;
	  bits[1] = (byte & 64) > 0;
	  bits[2] = (byte & 32) > 0;
	  bits[3] = (byte & 16) > 0;
	  bits[4] = (byte & 8) > 0;
	  bits[5] = (byte & 4) > 0;
	  bits[6] = (byte & 2) > 0;
	  bits[7] = (byte & 1) > 0;
}

void bytes_to_bits(uint8_t *bytes, size_t bytes_length, bit_t *bits, size_t bits_length)
{
    uint16_t i = 0;
    for (; i < min(bits_length/8, bytes_length); i++)
        byte_to_bits(bytes[i], &bits[i * 8]);
}

void _dmr_dump_hex(void *mem, size_t len, const char *file, size_t line)
{
    size_t i, j;

    if (file == NULL)
        printf("hex dump of %p+%zu:\n", mem, len);
    else
        printf("hex dump of %p+%zu in %s:%zu:\n", mem, len, file, line);

    for(i = 0; i < len + ((len % HEXDUMP_COLS) ? (HEXDUMP_COLS - len % HEXDUMP_COLS) : 0); i++) {
        /* print offset */
        if (i % HEXDUMP_COLS == 0) {
            printf("0x%06zx: ", i);
        }

        /* print hex data */
        if (i < len) {
            printf("%02x ", 0xFF & ((char*)mem)[i]);
        } else { /* end of block, just aligning for ASCII dump */
            printf("   ");
        }

        /* print ASCII dump */
        if (i % HEXDUMP_COLS == (HEXDUMP_COLS - 1)) {
            for (j = i - (HEXDUMP_COLS - 1); j <= i; j++) {
                if (j >= len - 1) { /* end of block, not really printing */
                    putchar(' ');
                } else if (isprint(((char*)mem)[j])) { /* printable char */
                    putchar(0xff & ((char*)mem)[j]);
                } else { /* other char */
                    putchar('.');
                }
            }
            putchar('\n');
        }
    }

    fflush(stdout);
}
