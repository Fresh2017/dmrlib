#include "common/uint.h"

PRIVATE void xuint8_pack(uint8_t *buf, uint8_t in)
{
    *(buf + 0) = xuint[(in >>  4)];
    *(buf + 1) = xuint[(in & 0xf)];
}
