#include "common/byte.h"

PRIVATE bool byte_equal_safe(const uint8_t *a, const uint8_t *b, size_t len)
{
    size_t i;
    uint8_t r = 0;
    for (i = 0; i < len; i++) {
        r |= (a[i] ^ b[i]);
    }
    return r == 0;
}
