#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include "dmr/log.h"
#include "dmr/bits.h"
#include "dmr/type.h"
#include "dmr/thread.h"

DMR_PRV static const char *dmr_log_priority_names[] = {
    "NULL",
    "trace",
    "debug",
    "info",
    "warn",
    "error",
    "critical"
};
DMR_PRV static const char *dmr_log_priority_tags[] = {
    NULL,
    "TRACE",
    "DEBUG",
    "INFO ",
    "WARN ",
    "ERROR",
    "FATAL",
    "UNDEF"
};
DMR_PRV static const char *dmr_log_priority_tags_colored[] = {
    NULL,
    "\x1b[1;36mTRACE\x1b[0m",
    "\x1b[1;34mDEBUG\x1b[0m",
    "\x1b[1;37mINFO \x1b[0m",
    "\x1b[1;31mWARN \x1b[0m",
    "\x1b[1;31mERROR\x1b[0m",
    "\x1b[1;31mFATAL\x1b[0m",
    "\x1b[1;31mUNDEF\x1b[0m"
};

DMR_PRV static bool log_color = true;
DMR_PRV static dmr_log_priority_t log_priority = DMR_LOG_PRIORITY_INFO;
DMR_PRV static const char *log_prefix = "";
DMR_PRV static void *log_mem = NULL;

DMR_PRV static void log_stderr(void *mem, dmr_log_priority_t priority, const char *msg)
{
    DMR_UNUSED(mem);
    const char *tag;

    if (log_color) {
        if (priority < DMR_LOG_PRIORITIES)
            tag = dmr_log_priority_tags_colored[priority];
        else
            tag = dmr_log_priority_tags_colored[DMR_LOG_PRIORITIES];
    } else {
        if (priority < DMR_LOG_PRIORITIES)
            tag = dmr_log_priority_tags[priority];
        else
            tag = dmr_log_priority_tags[DMR_LOG_PRIORITIES];
    }

#if defined(DMR_PLATFORM_WINDOWS)
    SYSTEMTIME lt;
    GetLocalTime(&lt);
    fprintf(stderr, "%s[%08lx][%s] %04d-%02d-%02d %02d:%02d:%02d %s\n",
        log_prefix, dmr_thread_id(NULL), tag,
        lt.wYear, lt.wMonth, lt.wDay, lt.wHour, lt.wMinute, lt.wSecond, msg);
#else
    time_t t = time(NULL);
    struct tm *tm = malloc(sizeof(*tm));
    if (tm == NULL) {
        fprintf(stderr, "out of memory while trying to render log message!\n");
        fprintf(stderr, "message was: %s\n", msg);
        fflush(stderr);
        return;
    }
    char tbuf[21];
    memset(tbuf, 0, sizeof(tbuf));
    localtime_r(&t, tm);
    strftime(tbuf, 20, DMR_LOG_TIME_FORMAT, tm);
    free(tm);
    fprintf(stderr, "%s%s [%s] %s\n",
        log_prefix, tbuf, tag, msg);
#endif
    fflush(stderr);
}

DMR_PRV static dmr_log_cb_t log_cb = log_stderr;

DMR_API bool dmr_log_color(void)
{
    return log_color;
}

DMR_API void dmr_log_color_set(bool enabled)
{
    log_color = enabled;
}

DMR_API dmr_log_priority_t dmr_log_priority(void)
{
    return log_priority;
}

DMR_API void dmr_log_priority_set(dmr_log_priority_t priority)
{
    dmr_log_priority_t old_priority = log_priority;
    if (priority == old_priority)
        return;

#if defined(DMR_TRACE)
    log_priority = min(DMR_LOG_PRIORITIES - 1, max(priority, DMR_LOG_PRIORITY_TRACE));
#elif defined(DMR_DEBUG)
    log_priority = min(DMR_LOG_PRIORITIES - 1, max(priority, DMR_LOG_PRIORITY_DEBUG));
#else
    log_priority = min(DMR_LOG_PRIORITIES - 1, max(priority, DMR_LOG_PRIORITY_INFO));
#endif
    dmr_log_debug("log: priority changed %s -> %s",
        dmr_log_priority_names[old_priority],
        dmr_log_priority_names[log_priority]);
}

DMR_API const char *dmr_log_prefix(void)
{
    return log_prefix;
}

DMR_API void dmr_log_prefix_set(const char *prefix)
{
    log_prefix = prefix;
}

DMR_API void dmr_log_priority_reset(void)
{
    log_priority = DMR_LOG_PRIORITY_INFO;
}

DMR_API void dmr_log(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    dmr_log_messagev(log_priority, fmt, ap);
    va_end(ap);
}

DMR_API void _dmr_log_trace(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    dmr_log_messagev(DMR_LOG_PRIORITY_TRACE, fmt, ap);
    va_end(ap);
}

DMR_API void _dmr_log_debug(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    dmr_log_messagev(DMR_LOG_PRIORITY_DEBUG, fmt, ap);
    va_end(ap);
}

DMR_API void dmr_log_info(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    dmr_log_messagev(DMR_LOG_PRIORITY_INFO, fmt, ap);
    va_end(ap);
}

DMR_API void dmr_log_warn(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    dmr_log_messagev(DMR_LOG_PRIORITY_WARN, fmt, ap);
    va_end(ap);
}

DMR_API void dmr_log_error(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    dmr_log_messagev(DMR_LOG_PRIORITY_ERROR, fmt, ap);
    va_end(ap);
}

DMR_API void dmr_log_critical(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    dmr_log_messagev(DMR_LOG_PRIORITY_CRITICAL, fmt, ap);
    va_end(ap);
}

DMR_API void dmr_log_message(dmr_log_priority_t priority, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    dmr_log_messagev(priority, fmt, ap);
    va_end(ap);
}

DMR_API void dmr_log_messagev(dmr_log_priority_t priority, const char *fmt, va_list ap)
{
    char *msg;
    size_t len;

    if (log_cb == NULL)
        return;

    if (priority < log_priority)
        return;

    msg = malloc(DMR_LOG_MESSAGE_MAX);  /* allocate on stack */
    if (msg == NULL)
        return;

    vsnprintf(msg, DMR_LOG_MESSAGE_MAX - 1, fmt, ap);

    len = strlen(msg);
    if (len > 0 && msg[len - 1] == '\n') {
        msg[len--] = 0;
        if (len > 0 && msg[len - 1] == '\r')
            msg[len--] = 0;
    }

    log_cb(log_mem, priority, msg);
    free(msg);
}

DMR_API void dmr_log_cb_get(dmr_log_cb_t *cb, void **mem)
{
    if (cb)
        *cb = log_cb;

    if (mem)
        *mem = log_mem;
}

DMR_API void dmr_log_cb(dmr_log_cb_t cb, void *mem)
{
    log_cb = cb;
    log_mem = mem;
}
