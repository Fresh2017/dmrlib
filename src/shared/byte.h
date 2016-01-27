#ifndef _SHARED_BYTE_H
#define _SHARED_BYTE_H

#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>

#if defined(__cplusplus)
extern "C" {
#endif

int8_t byte_cmp(uint8_t *a, uint8_t *b, size_t len);
void byte_copy(void *restrict dst, void *restrict src, size_t len);
bool byte_equal_safe(uint8_t *a, uint8_t *b, size_t len);
void byte_zero(void *buf, size_t len);

/* Do not use to compare passwords, see byte_equal_safe */
#define byte_equal(a, b, s) (bool)(!byte_cmp((a), (b), (s)))

#if defined(__cplusplus)
}
#endif

#endif // _SHARED_BYTE_H
