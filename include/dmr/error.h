/**
 * @file
 * @brief Error handling.
 */
#ifndef _DMR_ERROR_H
#define _DMR_ERROR_H

#include <errno.h>
#include <stdarg.h>
#include <dmr/config.h>
#include <dmr/log.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DMR_ERR_MAX_STRLEN  128
#define DMR_ERR_MAX_ARGS    5

typedef enum {
    DMR_ENOMEM      = 0x01,
    DMR_EINVAL      = 0x02,
    DMR_EWRITE      = 0x11,
    DMR_EREAD       = 0x12,
    DMR_ARG         = 0x21,
    DMR_UNSUPPORTED = 0x7f,
    DMR_LASTERROR   = 0xff
} dmr_errno;

typedef struct {
    int                no;
    char               msg[DMR_ERR_MAX_STRLEN];
    dmr_log_priority_t priority;
} dmr_error_t;

/**
 * Set an (numeric) error.
 * @returns int Always -1.
 */
extern int dmr_error(dmr_errno error);
/**
 * Set an error message.
 * @returns int Always -1.
 */
extern int dmr_error_set(const char *fmt, ...);
/** Get the last error. */
extern const char *dmr_error_get(void);
/** Clear the last error. */
extern void dmr_error_clear(void);

#define DMR_OOM()          dmr_error(DMR_ENOMEM);
#define DMR_ERROR_ARG(arg) dmr_error_set("argument \"%s\" is invalid", (arg))

#ifdef __cplusplus
}
#endif

#endif // _DMR_ERROR_H
