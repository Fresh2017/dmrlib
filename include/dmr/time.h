#ifndef _DMR_TIME_H
#define _DMR_TIME_H

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/time.h>
#include <inttypes.h>

uint32_t dmr_time_since(struct timeval tv);
uint32_t dmr_time_ms_since(struct timeval tv);

#ifdef __cplusplus
}
#endif

#endif // _DMR_TIME_H
