#include <stdbool.h>
#include <dmr/crc.h>

void dmr_crc9(uint16_t *crc, uint8_t byte, uint8_t bitlen)
{
    uint8_t i, v = 0x80;
    bool xor;
    for (i = 0; i < bitlen; i++) {
        v >>= 1;
    }
    for (i = 0; i < 8; i++) {
        xor = ((*crc) & 0x0100) == 0x0100;
        (*crc) = (*crc << 1) & 0x01ff;
        if (byte & v) {
            (*crc)++;
        }
        if (xor) {
            (*crc) ^= 0x59;
        }
        v >>= 1;
    }
}

/* Clears the CRC shift registers. */
void dmr_crc9_finish(uint16_t *crc, uint8_t bitlen)
{
    uint8_t i;
    bool xor;
    for (i = 0; i < bitlen; i++) {
        xor = ((*crc) & 0x0100) == 0x0100;
        (*crc) = (*crc << 1) & 0x01ff;
        if (xor) {
            (*crc) ^= 0x59;
        }
    }
}

void dmr_crc16(uint16_t *crc, uint8_t byte)
{
    uint8_t i, v = 0x80;
    bool xor;
    for (i = 0; i < 8; i++) {
        xor = ((*crc) & 0x8000) == 0x8000;
        (*crc) <<= 1;
        if (byte & v) {
            (*crc)++;
        }
        if (xor) {
            (*crc) ^= 0x1021;
        }
        v >>= 1;
    }
}

/* Clears the CRC shift registers. */
void dmr_crc16_finish(uint16_t *crc)
{
    uint8_t i;
    bool xor;
    for (i = 0; i < 8; i++) {
        xor = ((*crc) & 0x8000) == 0x8000;
        (*crc) <<= 1;
        if (xor) {
            (*crc) ^= 0x1021;
        }
    }
}

void dmr_crc32(uint32_t *crc, uint8_t byte)
{
    uint8_t i, v = 0x80;
    bool xor;
    for (i = 0; i < 8; i++) {
        xor = ((*crc) & 0x80000000) == 0x80000000;
        (*crc) <<= 1;
        if (byte & v) {
            (*crc)++;
        }
        if (xor) {
            (*crc) ^= 0x04c11db7;
        }
        v >>= 1;
    }
}

/* Clears the CRC shift registers. */
void dmr_crc32_finish(uint32_t *crc)
{
    uint8_t i;
    bool xor;
    for (i = 0; i < 8; i++) {
        xor = ((*crc) & 0x80000000) == 0x80000000;
        (*crc) <<= 1;
        if (xor) {
            (*crc) ^= 0x04c11db7;
        }
    }
}
