#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include "common/debug.h"

static void debug_handler_default(const char *fmt, ...)
{
    va_list args;
	va_start(args, fmt);
	if (getenv("COMMON_DEBUG")) {
		fputs("common: ", stderr);
		vfprintf(stderr, fmt, args);
	}
	va_end(args);
}

void (*debug_handler)(const char *fmt, ...) = debug_handler_default;

char *debugip4(ip4_t ip)
{
    static char host[FORMAT_IP4_LEN];
    format_ip4(host, ip);
    return host;
}

char *debugip6(ip6_t ip)
{
    static char host[FORMAT_IP6_LEN];
    format_ip6(host, ip);
    return host;
}
