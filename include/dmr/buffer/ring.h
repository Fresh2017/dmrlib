#ifndef _DMR_BUFFER_RING_H
#define _DMR_BUFFER_RING_H

#include <inttypes.h>
#include <ctype.h>
#include <stdbool.h>

typedef struct {
    uint8_t *buffer;
    uint8_t *head;
    uint8_t *tail;
    size_t  size;
} dmr_ring_t;

extern dmr_ring_t *dmr_ring_new(size_t size);
extern void dmr_ring_reset(dmr_ring_t *ring);
extern void dmr_ring_free(dmr_ring_t *ring);
extern size_t dmr_ring_capacity(dmr_ring_t *ring);
extern size_t dmr_ring_buffer_size(dmr_ring_t *ring);
extern size_t dmr_ring_bytes_free(dmr_ring_t *ring);
extern size_t dmr_ring_bytes_used(dmr_ring_t *ring);
extern bool dmr_ring_full(dmr_ring_t *ring);
extern bool dmr_ring_empty(dmr_ring_t *ring);
extern size_t dmr_ring_memset(dmr_ring_t *ring, int c, size_t len);
extern size_t dmr_ring_write(dmr_ring_t *ring, uint8_t *buffer, size_t len);

#endif // _DMR_BUFFER_RING_H
