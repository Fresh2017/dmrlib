#include "common/uint.h"

PRIVATE void xuint16_pack(uint8_t *buf, uint16_t in)
{
    *(buf + 0) = xuint[(in >> 12) & 0x0f];
    *(buf + 1) = xuint[(in >>  8) & 0x0f];
    *(buf + 2) = xuint[(in >>  4) & 0x0f];
    *(buf + 3) = xuint[(in >>  0) & 0x0f];
}

PRIVATE void xuint16_pack_le(uint8_t *buf, uint16_t in)
{
    *(buf + 0) = xuint[(in >>  4) & 0x0f];
    *(buf + 1) = xuint[(in >>  0) & 0x0f];
    *(buf + 2) = xuint[(in >> 12) & 0x0f];
    *(buf + 3) = xuint[(in >>  8) & 0x0f];
}
