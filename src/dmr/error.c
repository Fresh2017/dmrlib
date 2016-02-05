#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "dmr/error.h"
#include "dmr/log.h"
#include "dmr/thread.h"

DMR_PRV static _dmr_thread_local dmr_error_t error;
DMR_PRV static const char *dmr_error_enomem      = "out of memory";
DMR_PRV static const char *dmr_error_einval      = "invalid argument";
DMR_PRV static const char *dmr_error_ewrite      = "write failed";
DMR_PRV static const char *dmr_error_eread       = "read failed";
DMR_PRV static const char *dmr_error_unsupported = "not supported";
DMR_PRV static const char *dmr_error_unknown     = "unkonwn error";

DMR_API int _dmr_error_set(const char *msg, dmr_log_priority_t priority)
{
    error.priority = priority;
    strcpy(error.msg, msg);
    return -1;
}

DMR_API int dmr_error(dmr_errno err)
{
    error.no = err;

    switch (err) {
    case DMR_ENOMEM:
        return _dmr_error_set(dmr_error_enomem, DMR_LOG_PRIORITY_CRITICAL);
    case DMR_EINVAL:
        return _dmr_error_set(dmr_error_einval, DMR_LOG_PRIORITY_ERROR);
    case DMR_EWRITE:
        return _dmr_error_set(dmr_error_ewrite, DMR_LOG_PRIORITY_ERROR);
    case DMR_EREAD:
        return _dmr_error_set(dmr_error_eread, DMR_LOG_PRIORITY_ERROR);
    case DMR_UNSUPPORTED:
        return _dmr_error_set(dmr_error_unsupported, DMR_LOG_PRIORITY_ERROR);
    case DMR_LASTERROR:
        return -1;
    default:
        return _dmr_error_set(dmr_error_unknown, DMR_LOG_PRIORITY_ERROR);
    }
}

DMR_API int dmr_error_set(const char *fmt, ...)
{
    va_list ap;

    if (fmt == NULL)
        return -1;

    va_start(ap, fmt);
    vsnprintf(error.msg, DMR_ERR_MAX_STRLEN, fmt, ap);
    va_end(ap);

    return -1;
}

DMR_API const char *dmr_error_get(void)
{
    if (strlen(error.msg) == 0)
        return "no error";
    return error.msg;
}

DMR_API void dmr_error_clear(void)
{
    memset(error.msg, 0, DMR_ERR_MAX_STRLEN);
}
