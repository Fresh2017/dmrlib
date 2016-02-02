#ifndef _COMMON_FORMAT_H
#define _COMMON_FORMAT_H

#include <stddef.h>
#include "common/socket.h"

#if defined(__cplusplus)
extern "C" {
#endif

#define FORMAT_IP4_LEN 20
#define FORMAT_IP6_LEN 40

unsigned int format_ip4(char *dst, const ip4_t ip);
char *       format_ip4s(const ip4_t ip);
unsigned int format_ip6(char *dst, const ip6_t ip);
char *       format_ip6s(const ip6_t ip);
int 		 format_path_canonical(char *dst, size_t len, const char *path);
const char * format_path_ext(const char *path);
void 		 format_path_join(char *dst, size_t len, const char *dir, const char *file);
char 		 format_path_sep(void);
size_t       format_ulong(char *dst, unsigned long i);
size_t       format_xlong(char *dst, unsigned long i);

#if defined(__cplusplus)
}
#endif

#endif // _COMMON_FORMAT_H
