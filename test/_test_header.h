#include <stdio.h>
#include <string.h>
#include <time.h>
#include <dmr/error.h>
#include <dmr/log.h>
#include <dmr/malloc.h>
#include <dmr/packet.h>

typedef bool (*test_func)(void);

typedef struct {
    char      *name;
    test_func test;
} test_t;

dmr_packet_t *test_packet(void)
{
    dmr_packet_t *packet = dmr_malloc(dmr_packet_t);
    if (packet == NULL) {
        printf("out of memory\n");
        exit(1);
    }
    memset(packet, 0, sizeof(dmr_packet_t));
    return packet;
}

bool fail(char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
    return false;
}

bool pass(char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
    return true;
}

#define eq(expr, fmt, ...) if(!(expr))     return fail(fmt, ## __VA_ARGS__)
#define ne(expr, fmt, ...) if( (expr))     return fail(fmt, ## __VA_ARGS__)
#define go(func, fmt, ...) if((func) != 0) return fail(fmt, ## __VA_ARGS__)
