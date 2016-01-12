/**
 * @file
 * @brief Logging.
 */
#ifndef _DMR_LOG_H
#define _DMR_LOG_H

#include <stdarg.h>
#include <stdio.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <dmr/platform.h>

#define DMR_LOG_MESSAGE_MAX 1024
#if defined(DMR_PLATFORM_WINDOWS)
#define DMR_LOG_TIME_FORMAT "%Y-%m-%dT%H:%M:%SZ"
#else
#define DMR_LOG_TIME_FORMAT "%F %T"
#endif // DMR_PLATFORM_WINDOWS

#define DMR_LOG_BOOL(x) (x ? "true" : "false")

typedef enum {
    DMR_LOG_PRIORITY_TRACE = 1,
    DMR_LOG_PRIORITY_DEBUG,
    DMR_LOG_PRIORITY_INFO,
    DMR_LOG_PRIORITY_WARN,
    DMR_LOG_PRIORITY_ERROR,
    DMR_LOG_PRIORITY_CRITICAL,
    DMR_LOG_PRIORITIES
} dmr_log_priority_t;

extern bool dmr_log_color(void);
extern void dmr_log_color_set(bool enabled);
extern dmr_log_priority_t dmr_log_priority(void);
extern void dmr_log_priority_set(dmr_log_priority_t priority);
extern void dmr_log_priority_reset(void);
extern const char *dmr_log_prefix(void);
extern void dmr_log_prefix_set(const char *prefix);
extern void dmr_log(const char *fmt, ...);
extern void dmr_log_trace(const char *fmt, ...);
extern void dmr_log_debug(const char *fmt, ...);
extern void dmr_log_info(const char *fmt, ...);
extern void dmr_log_warn(const char *fmt, ...);
extern void dmr_log_error(const char *fmt, ...);
extern void dmr_log_critical(const char *fmt, ...);
extern void dmr_log_message(dmr_log_priority_t priority, const char *fmt, ...);
extern void dmr_log_messagev(dmr_log_priority_t priority, const char *fmt, va_list ap);

typedef void (*dmr_log_cb_t)(void *mem, dmr_log_priority_t priority, const char *msg);
extern void dmr_log_cb_get(dmr_log_cb_t *cb, void **mem);
extern void dmr_log_cb(dmr_log_cb_t cb, void *mem);

#ifdef __cplusplus
}
#endif

#endif // _DMR_LOG_H
