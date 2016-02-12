
/**
 * @file   Raw byte buffers.
 * @brief  ...
 * @author Wijnand Modderman-Lenstra PD0MZ
 */
#ifndef _DMR_RAW_H
#define _DMR_RAW_H

#include <dmr.h>
#include <dmr/type.h>
#include <dmr/queue.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct dmr_raw {
    uint8_t                  *buf;       /* actual buffer */
    int64_t                  allocated;  /* bytes */
    uint64_t                 len;        /* bytes */
    DMR_TAILQ_ENTRY(dmr_raw) entries;    /* tail queue */
} dmr_raw;

typedef struct {
    DMR_TAILQ_HEAD(,dmr_raw) head;     /* tail queue head */
    size_t                   limit;    /* tail queue size limit */
} dmr_rawq;

typedef void (*dmr_raw_cb)(dmr_raw *raw, void *userdata);

/** Setup a new raw buffer. */
extern dmr_raw *dmr_raw_new(uint64_t len);

/** Destroy a raw buffer. */
extern void dmr_raw_free(dmr_raw *raw);

/** Add to the raw buffer. */
extern int dmr_raw_add(dmr_raw *raw, const void *buf, size_t len);

/** Add to the raw buffer and hex encode. */
extern int dmr_raw_add_hex(dmr_raw *raw, const void *buf, size_t len);

/** Add an uint8_t to the buffer. */
extern int dmr_raw_add_uint8(dmr_raw *raw, uint8_t in);

/** Add an uint16_t to the buffer. */
extern int dmr_raw_add_uint16(dmr_raw *raw, uint16_t in);

/** Add an uint24_t to the buffer. */
extern int dmr_raw_add_uint24(dmr_raw *raw, uint24_t in);

/** Add an uint32_t to the buffer. */
extern int dmr_raw_add_uint32(dmr_raw *raw, uint32_t in);

/** Add an uint32_t to the buffer (little endian). */
extern int dmr_raw_add_uint32_le(dmr_raw *raw, uint32_t in);

/** Add an uint64_t to the buffer. */
extern int dmr_raw_add_uint64(dmr_raw *raw, uint64_t in);

/** Add a hex encoded uint8_t to the buffer. */
extern int dmr_raw_add_xuint8(dmr_raw *raw, uint8_t in);

/** Add a hex encoded uint16_t to the buffer. */
extern int dmr_raw_add_xuint16(dmr_raw *raw, uint16_t in);

/** Add a hex encoded uint24_t to the buffer. */
extern int dmr_raw_add_xuint24(dmr_raw *raw, uint24_t in);

/** Add a hex encoded uint32_t to the buffer. */
extern int dmr_raw_add_xuint32(dmr_raw *raw, uint32_t in);

/** Add a hex encoded uint32_t to the buffer (little endian). */
extern int dmr_raw_add_xuint32_le(dmr_raw *raw, uint32_t in);

/** Add a hex encoded uint64_t to the buffer. */
extern int dmr_raw_add_xuint64(dmr_raw *raw, uint64_t in);

/** Add a formatted string to the buffer. */
extern int dmr_raw_addf(dmr_raw *raw, size_t len, const char *fmt, ...);

/** Resize a raw buffer. */
extern int dmr_raw_grow(dmr_raw *raw, uint64_t len);

/** Resize a raw buffer if `add` number of bytes can't be added. */
extern int dmr_raw_grow_add(dmr_raw *raw, uint64_t add);

/** Reset a raw buffer. */
extern int dmr_raw_reset(dmr_raw *raw);

/** Zero a raw buffer, maitaining the size. */
extern int dmr_raw_zero(dmr_raw *raw);

/** Setup a new raw queue.
 * A limit of 0 means unlimited queue size. */
extern dmr_rawq *dmr_rawq_new(size_t limit);

/** Destroy a raw queue. */
extern void dmr_rawq_free(dmr_rawq *rawq);

/** Add a buffered element to the raw queue. */
extern int dmr_rawq_add(dmr_rawq *rawq, dmr_raw *raw);

/** Add a buffer to the raw queue. */
extern int dmr_rawq_addb(dmr_rawq *rawq, uint8_t *buf, size_t len);

/** Iterate over all the items in a raw queue. */
extern int dmr_rawq_each(dmr_rawq *rawq, dmr_raw_cb cb, void *userdata);

/** Check if the rawq is empty. */
extern bool dmr_rawq_empty(dmr_rawq *rawq);

/** Size of the rawq. */
extern size_t dmr_rawq_size(dmr_rawq *rawq);

/** Shift the first element from the raw queue. */
extern dmr_raw *dmr_rawq_shift(dmr_rawq *rawq);

/** Prepend a raw buffer to the raw queue. */
extern int dmr_rawq_unshift(dmr_rawq *rawq, dmr_raw *raw);

#ifdef __cplusplus
}
#endif

#endif // _DMR_RAW_H
