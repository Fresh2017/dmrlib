#include "common/byte.h"

bool byte_equal_safe(uint8_t *a, uint8_t *b, size_t len)
{
    size_t i;
    uint8_t r = 0;
    for (i = 0; i < len; i++) {
        r |= (a[i] ^ b[i]);
    }
    return r == 0;
}
