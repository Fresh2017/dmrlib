#include "common/byte.h"

int8_t byte_cmp(uint8_t *a, uint8_t *b, size_t len)
{
    size_t i;
    for (i = 0; i < len; i++) {
        int16_t r = a[i] - b[i];
        if (r != 0) {
            return r;
        }
    }
    return 0;
}
