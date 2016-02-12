/**
 * @file   C macro's
 * @brief  C macro's to aid common patterns.
 * @author Wijnand Modderman-Lenstra PD0MZ
 */
#ifndef _DMR_C_H
#define _DMR_C_H

#include <dmr/error.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DMR_CHECK_IF_NULL(x,e) do { \
    if ((x) == NULL) { \
        dmr_error(e); \
        return NULL; \
    } \
} while(0)

#define DMR_CHECK_IF_NULL_FREE(x,e,f) do { \
    if ((x) == NULL) { \
        TALLOC_FREE(f); \
        dmr_error(e); \
        return NULL; \
    } \
} while(0)

#define DMR_ERROR_IF_NULL(x,e) do { \
    if ((x) == NULL) { \
        return dmr_error(e); \
    } \
} while(0)

#define DMR_ERROR_IF_NULL_FREE(x,e,f) do { \
    if ((x) == NULL) { \
        TALLOC_FREE(f); \
        return dmr_error(e); \
    } \
} while(0)

#ifdef __cplusplus
}
#endif

#endif // _DMR_C_H
