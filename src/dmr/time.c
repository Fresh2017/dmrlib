#include <stddef.h>
#include "dmr/time.h"

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
