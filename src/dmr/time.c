#include <stddef.h>
#include "dmr/time.h"

uint32_t dmr_time_since(struct timeval tv)
{
    struct timeval now;
    gettimeofday(&now, NULL);
    return (uint32_t)(now.tv_sec - tv.tv_sec);
}

double dmr_time_ms_since(struct timeval tv)
{
    struct timeval now;
    gettimeofday(&now, NULL);
    double delta;
    delta  = (now.tv_sec  - tv.tv_sec)  / 1000.0;
    delta += (now.tv_usec - tv.tv_usec) * 1000.0;
    return delta;
}
