#include <string.h>
#include <stdlib.h>

#include "dmr/bits.h"
#include "dmr/buffer/ring.h"

dmr_ring_t *dmr_ring_new(size_t size)
{
    dmr_ring_t *ring = malloc(sizeof(dmr_ring_t));
    if (ring == NULL)
        return NULL;

    ring->size = size + 1;
    ring->buffer = malloc(ring->size);
    if (ring->buffer) {
        memset(ring->buffer, 0, ring->size);
        dmr_ring_reset(ring);
    } else {
        free(ring);
        return NULL;
    }

    return ring;
}

void dmr_ring_reset(dmr_ring_t *ring)
{
    ring->head = ring->tail = ring->buffer;
}

void dmr_ring_free(dmr_ring_t *ring)
{
    if (ring == NULL)
        return;

    free(ring->buffer);
    free(ring);
}

size_t dmr_ring_capacity(dmr_ring_t *ring)
{
    return dmr_ring_buffer_size(ring) - 1;
}

static const uint8_t * dmr_ring_end(const dmr_ring_t *ring)
{
    return ring->buffer + dmr_ring_buffer_size((dmr_ring_t *)ring);
}

size_t dmr_ring_buffer_size(dmr_ring_t *ring)
{
    return ring->size;
}

size_t dmr_ring_bytes_free(dmr_ring_t *ring)
{
    if (ring->head >= ring->tail)
        return dmr_ring_capacity(ring) - (ring->head - ring->tail);
    else
        return ring->tail - ring->head - 1;
}

size_t dmr_ring_bytes_used(dmr_ring_t *ring)
{
    return dmr_ring_capacity(ring) - dmr_ring_bytes_free(ring);
}

bool dmr_ring_full(dmr_ring_t *ring)
{
    return dmr_ring_bytes_free(ring) == 0;
}

bool dmr_ring_empty(dmr_ring_t *ring)
{
    return dmr_ring_bytes_free(ring) == dmr_ring_capacity(ring);
}

static uint8_t *dmr_ring_nextp(dmr_ring_t *ring, const uint8_t *p)
{
    if (p < ring->buffer || p >= dmr_ring_end(ring))
        return NULL;
    return ring->buffer + ((++p - ring->buffer) % dmr_ring_buffer_size(ring));
}

size_t dmr_ring_memset(dmr_ring_t *ring, int c, size_t len)
{
    const uint8_t *end = dmr_ring_end(ring);
    size_t n = 0, count = min((size_t)len, dmr_ring_buffer_size(ring));
    bool overflow = count > dmr_ring_bytes_free(ring);

    while (n != count) {
        /* don't copy beyond the end of the buffer */
        if (end >= ring->head)
            break;

        size_t i = min((size_t)(end - ring->head), (size_t)(count - n));
        memset(ring->head, c, i);
        ring->head += i;
        n += i;

        if (ring->head == end)
            ring->head = ring->buffer;
    }

    if (overflow) {
        ring->tail = dmr_ring_nextp(ring, ring->head);
    }

    return n;
}

size_t dmr_ring_write(dmr_ring_t *ring, uint8_t *buffer, size_t len)
{
    size_t i;

    if (len > dmr_ring_bytes_free(ring))
        return 0;

    const uint8_t *end = dmr_ring_end(ring);
    for (i = 0; i < len; i++) {
        *ring->head = buffer[i];
        ring->head++;

        if (ring->head == end)
            ring->head = ring->buffer;
    }

    return len;
}
