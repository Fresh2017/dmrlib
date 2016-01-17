#include <stdio.h>
#include <string.h>
#include <time.h>
#include <dmr/error.h>
#include <dmr/log.h>
#include <dmr/malloc.h>
#include <dmr/packet.h>

static char *prog = NULL;

typedef bool (*test_func)(void);

typedef struct {
    char      *name;
    test_func test;
} test_t;

uint8_t flip_random_bit(uint8_t byte)
{
    static uint8_t single_bits[] = {
        0x01, 0x02, 0x04, 0x08,
        0x10, 0x20, 0x40, 0x80
    };
    return byte ^ single_bits[rand() % 8];
}

void flip_random_byte(uint8_t *buf, uint8_t len, uint8_t count)
{
    uint8_t i, j;
    for (i = 0; i < count; i++) {
        j = (rand() % len);
        buf[j] = flip_random_bit(buf[j]);
    }
}

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

uint8_t rv;
#define eq(expr, fmt, ...) if(!(expr))            return fail(fmt, ## __VA_ARGS__)
#define ne(expr, fmt, ...) if( (expr))            return fail(fmt, ## __VA_ARGS__)
#define go(func, fmt, ...) if((rv = (func)) != 0) return fail("return code %d" #fmt, rv, ## __VA_ARGS__)
/*
#define go(func, fmt, ...) do { if((rv = (func)) != 0) {     \
    printf("check returned %d, ", rv);                       \
    return fail(fmt, ## __VA_ARGS__);                        \
} while(0)
*/
