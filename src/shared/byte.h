#ifndef _SHARED_BYTE_H
#define _SHARED_BYTE_H

#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include "shared/config.h"

#if defined(__cplusplus)
extern "C" {
#endif

#if defined(__restrict__)
#define ___restrict __restrict__
#elif defined(__restrict)
#define ___restrict __restrict
#else
#define ___restrict
#endif

int8_t byte_cmp(uint8_t *a, uint8_t *b, size_t len);
void byte_copy(void *___restrict dst, void *___restrict src, size_t len);
bool byte_equal_safe(uint8_t *a, uint8_t *b, size_t len);
void byte_zero(void *buf, size_t len);

/* Do not use to compare passwords, see byte_equal_safe */
#define byte_equal(a, b, s) (bool)(!byte_cmp((a), (b), (s)))

#if defined(__cplusplus)
}
#endif

#endif // _SHARED_BYTE_H
