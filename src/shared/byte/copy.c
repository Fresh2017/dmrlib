#include "shared/byte.h"

void byte_copy(void *restrict dst, void *restrict src, size_t len)
{
    uint8_t *a = dst;
    const uint8_t *b = src;
    size_t i;
    for (i = 0; i < len; i++) {
        a[i] = b[i];
    }
}
