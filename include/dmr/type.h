#ifndef _DMR_TYPE_H
#define _DMR_TYPE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdbool.h>
#include <inttypes.h>

// Portable version
#define DMR_UNUSED(x) (void)(x)

#if defined __uint24
#define uint24_t __uint24
#else
#define uint24_t uint32_t
#endif

#ifdef __cplusplus
}
#endif

#endif // _DMR_TYPE_H
