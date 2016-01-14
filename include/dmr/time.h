#ifndef _DMR_TIME_H
#define _DMR_TIME_H

#include <sys/time.h>
#include <inttypes.h>

uint32_t dmr_time_since(struct timeval tv);
uint32_t dmr_time_ms_since(struct timeval tv);

#endif // _DMR_TIME_H
