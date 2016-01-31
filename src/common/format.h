#ifndef _COMMON_FORMAT_H
#define _COMMON_FORMAT_H

#include <stddef.h>
#include "common/socket.h"

#if defined(__cplusplus)
extern "C" {
#endif

unsigned int format_ip4(char *dst, const ip4_t ip);
unsigned int format_ip6(char *dst, const ip6_t ip);
size_t       format_ulong(char *dst, unsigned long i);
size_t       format_xlong(char *dst, unsigned long i);

#if defined(__cplusplus)
}
#endif

#endif // _COMMON_FORMAT_H
