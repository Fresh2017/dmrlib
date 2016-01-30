#include <stddef.h>
#include "dmr/time.h"
#include "dmr/platform.h"

#if defined(DMR_PLATFORM_WINDOWS)
static void timersub(const struct timeval* tvp, const struct timeval* uvp, struct timeval* vvp)
{
    vvp->tv_sec = tvp->tv_sec - uvp->tv_sec;
    vvp->tv_usec = tvp->tv_usec - uvp->tv_usec;
    if (vvp->tv_usec < 0) {
        --vvp->tv_sec;
        vvp->tv_usec += 1000000;
    }
}
#endif

uint32_t dmr_time_since(struct timeval tv)
{
    struct timeval now, res;
    gettimeofday(&now, NULL);
    timersub(&now, &tv, &res);
    return (uint32_t)(res.tv_sec);
}

uint32_t dmr_time_ms_since(struct timeval tv)
{
    struct timeval now, res;
    gettimeofday(&now, NULL);
    timersub(&now, &tv, &res);
    return (res.tv_sec * 1000) + ((res.tv_usec + 500) / 1000);
}
