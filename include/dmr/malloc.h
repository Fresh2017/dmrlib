/**
 * @file
 * @brief  ...
 * @author Wijnand Modderman-Lenstra PD0MZ
 */
#ifndef _DMR_MALLOC_H
#define _DMR_MALLOC_H

#include <errno.h>
#include <talloc.h>
#include <dmr/log.h>

/** Allocate a 0-initizialized structure with parent context. */
#define dmr_palloc(ctx,type)                talloc_zero(ctx,type)

/** Allocate a 0-initizialized untyped buffer with parent context. */
#define dmr_palloc_size(ctx,size)           talloc_zero_size(ctx,size)

/** Allocate a 0-initizialized structure. */
#define dmr_malloc(type)                    dmr_palloc(NULL,type)

/** Allocate a 0-initizialized untyped buffer. */
#define dmr_malloc_size(size)               dmr_palloc_size(NULL,size)

/** Resize an untyped buffer with parent context. */
#define dmr_realloc(ctx,ptr,type,size)      talloc_realloc(ctx, ptr, type, size)

/** Duplicate a string with parent context. */
#define dmr_strdup(ctx,str)                 talloc_strdup(ctx, str)

/** Free a previously allocated structure. */
#define dmr_free(ctx)                       TALLOC_FREE(ctx)

#define DMR_NULL_CHECK(expr)                        \
    if ((expr) == NULL) {                           \
        dmr_log_critical("out of memory");          \
        errno = ENOMEM;                             \
        return NULL;                                \
    }

#define DMR_NULL_CHECK_FREE(expr,var)               \
    if ((expr) == NULL) {                           \
        dmr_log_critical("out of memory");          \
        errno = ENOMEM;                             \
        dmr_free(var);                              \
        return NULL;                                \
    }

/** Allocate a 0-initizialized structure with parent context, check for NULL.
 *  If the dmr_palloc result is NULL, set OOM error and return NULL. */
#define DMR_PALLOC_CHECK(type,var,parent)               \
    type *var;                                          \
    DMR_NULL_CHECK(var = dmr_palloc(parent, type))

/** Allocate a 0-initizialized structure with, check for NULL.
 *  If the dmr_palloc result is NULL, set OOM error and return NULL. */
#define DMR_MALLOC_CHECK(type,var)  DMR_PALLOC_CHECK(type,var,NULL)

#endif // _DMR_MALLOC_H
