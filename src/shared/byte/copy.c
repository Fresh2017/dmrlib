#include "shared/byte.h"

#if defined(HAVE_RESTRICT)
void byte_copy(void *restrict dst, void *restrict src, size_t len)
#else
void byte_copy(void *dst, void *src, size_t len)
#endif
{
    uint8_t *a = dst;
    const uint8_t *b = src;
    size_t i;
    for (i = 0; i < len; i++) {
        a[i] = b[i];
    }
}
