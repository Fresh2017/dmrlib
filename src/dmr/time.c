#include <stddef.h>
#include "dmr/time.h"

uint32_t dmr_time_since(struct timeval tv)
{
    struct timeval now;
    gettimeofday(&now, NULL);
    return (uint32_t)(now.tv_sec - tv.tv_sec);
}

uint64_t dmr_time_ms_since(struct timeval tv)
{
    struct timeval now;
    gettimeofday(&now, NULL);
    uint16_t delta = 0;
    if (now.tv_usec > tv.tv_usec) {
        delta += (now.tv_usec - tv.tv_usec) / 1000UL;
    } else {
        delta += (tv.tv_usec - now.tv_usec) / 1000UL;
        now.tv_usec--;
    }
    delta += (now.tv_sec - tv.tv_sec) * 1000UL;
    return delta;
}
