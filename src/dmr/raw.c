#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "dmr/raw.h"
#include "dmr/malloc.h"
#include "common/byte.h"

DMR_PRV static inline uint32_t swap_uint32(uint32_t in)
{
    return (in << 24) | ((in << 8) & 0x00ff0000) | 
           ((in >> 8) & 0x0000ff00) | (in >> 24);
}

DMR_API dmr_raw *dmr_raw_new(uint64_t len)
{
    dmr_raw *raw = dmr_malloc(dmr_raw);
    DMR_CHECK_IF_NULL(raw, DMR_ENOMEM); 
    /* round up the allocation */
    len = (len + 127)&(-128ll);
    DMR_NULL_CHECK_FREE(raw->buf = dmr_palloc_size(raw, len), raw);
    raw->allocated = len;
    raw->len = 0;
    return raw;
}

DMR_API void dmr_raw_free(dmr_raw *raw)
{
    dmr_free(raw);
}

DMR_API int dmr_raw_add(dmr_raw *raw, const void *buf, size_t len)
{
    DMR_ERROR_IF_NULL(raw, DMR_EINVAL);
    if (dmr_raw_grow_add(raw, len) != 0)
        return dmr_error(DMR_LASTERROR);
    byte_copy(raw->buf + raw->len, buf, len);
    raw->len += len;
    return 0;
}

DMR_API int dmr_raw_add_hex(dmr_raw *raw, const void *buf, size_t len)
{
    DMR_ERROR_IF_NULL(raw, DMR_EINVAL);
    if (dmr_raw_grow_add(raw, len * 2) != 0)
        return dmr_error(DMR_LASTERROR);

    size_t i;
    uint8_t b;
    const uint8_t *tmp = buf;
    for (i = 0; i < len; i++) {
        b = *tmp++;
        raw->buf[raw->len++] = bhex[b >> 4];
        raw->buf[raw->len++] = bhex[b & 0x0f];
    }

    return 0;
}

DMR_API int dmr_raw_add_uint8(dmr_raw *raw, uint8_t in)
{
    DMR_ERROR_IF_NULL(raw, DMR_EINVAL);
    if (dmr_raw_grow_add(raw, 1) != 0) {
        return dmr_error(DMR_LASTERROR);
    }
    raw->buf[raw->len++] = in;
    return 0;
}

DMR_API int dmr_raw_add_xuint8(dmr_raw *raw, uint8_t in)
{
    DMR_ERROR_IF_NULL(raw, DMR_EINVAL);
    if (dmr_raw_grow_add(raw, 2) != 0) {
        return dmr_error(DMR_LASTERROR);
    }
    raw->buf[raw->len++] = bhex[(in >> 4)];
    raw->buf[raw->len++] = bhex[(in & 0x0f)];
    return 0;
}

#define AT(type) type ## _t
#define AU(type,size) \
DMR_API int dmr_raw_add_##type(dmr_raw *raw, AT(type) in) \
{ \
    DMR_ERROR_IF_NULL(raw, DMR_EINVAL); \
    if (dmr_raw_grow_add(raw, size) != 0) \
        return dmr_error(DMR_LASTERROR);\
    \
    size_t i; \
    for (i = 0; i < size; i++) { \
        raw->buf[raw->len++] = (in & 0xff); \
        in >>= 8; \
    } \
    return 0; \
}
AU(uint16, 2);
AU(uint24, 3);
AU(uint32, 4);
AU(uint64, 8);

DMR_API int dmr_raw_add_uint32_le(dmr_raw *raw, uint32_t in)
{
    return dmr_raw_add_uint32(raw, swap_uint32(in));
}

#define AX(type,size,fmt) \
DMR_API int dmr_raw_add_x##type(dmr_raw *raw, AT(type) in) \
{ \
    DMR_ERROR_IF_NULL(raw, DMR_EINVAL); \
    if (dmr_raw_grow_add(raw, size) != 0) \
        return dmr_error(DMR_LASTERROR);\
    \
    byte_zero(raw->buf + raw->len, size); \
    snprintf((char *)(raw->buf + raw->len), size, fmt, in); \
    raw->len += size; \
    return 0; \
}
AX(uint16, 4, "%04"PRIx16);
AX(uint24, 6, "%06"PRIx32);
AX(uint32, 8, "%08"PRIx32);
AX(uint64, 16, "%016"PRIx64);
#undef AX
#undef AU
#undef AT

DMR_API int dmr_raw_add_xuint32_le(dmr_raw *raw, uint32_t in)
{
    return dmr_raw_add_xuint32(raw, swap_uint32(in));
}

DMR_API int dmr_raw_addf(dmr_raw *raw, size_t len, const char *fmt, ...)
{
    DMR_ERROR_IF_NULL(raw, DMR_EINVAL);
    /* +1 because vsnprintf adds NULL bytes */
    if (dmr_raw_grow_add(raw, len + 1) != 0)
        return dmr_error(DMR_LASTERROR);

    char *dst = (char *)(raw->buf + raw->len);
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(dst, len + 1, fmt, ap);
    va_end(ap);
    raw->len += len;

    return 0;
}

DMR_API int dmr_raw_grow(dmr_raw *raw, uint64_t len)
{
    DMR_ERROR_IF_NULL(raw, DMR_EINVAL);
    if (len < (uint64_t)raw->allocated)
        return 0;

    /* round up allocation */
    len = (len + 127) & (-128ll);
    uint8_t *tmp = dmr_realloc(raw, raw->buf, uint8_t, len);
    DMR_ERROR_IF_NULL(tmp, DMR_ENOMEM);
    raw->buf = tmp;
    raw->allocated = len;
    byte_zero(raw->buf + raw->len, raw->allocated - raw->len);
    return 0;
}

DMR_API int dmr_raw_grow_add(dmr_raw *raw, uint64_t add)
{
    DMR_ERROR_IF_NULL(raw, DMR_EINVAL);
    return dmr_raw_grow(raw, raw->len + add);
}

DMR_API int dmr_raw_reset(dmr_raw *raw)
{
    DMR_ERROR_IF_NULL(raw, DMR_EINVAL);
    byte_zero(raw->buf, raw->allocated);
    raw->len = 0;
    return 0;
}

DMR_API uint64_t dmr_raw_size(dmr_raw *raw)
{
    if (raw == NULL)
        return 0;
    return raw->len;
}

DMR_API int dmr_raw_zero(dmr_raw *raw)
{
    DMR_ERROR_IF_NULL(raw, DMR_EINVAL);
    byte_zero(raw->buf, raw->len);
    return 0;
}

DMR_API dmr_rawq *dmr_rawq_new(size_t limit)
{
    DMR_MALLOC_CHECK(dmr_rawq, rawq);
    DMR_TAILQ_INIT(&rawq->head);
    rawq->limit = limit;
    return rawq;
}

DMR_API int dmr_rawq_add(dmr_rawq *rawq, dmr_raw *raw)
{
    DMR_ERROR_IF_NULL(rawq, DMR_EINVAL);
    DMR_ERROR_IF_NULL(raw, DMR_EINVAL);

    if (rawq->limit == 0 || dmr_rawq_size(rawq) < rawq->limit) {
        DMR_TAILQ_INSERT_TAIL(&rawq->head, raw, entries);
        return 0;
    }

    /* queue full */
    return -1;
}

DMR_API int dmr_rawq_addb(dmr_rawq *rawq, uint8_t *buf, size_t len)
{
    DMR_ERROR_IF_NULL(rawq, DMR_EINVAL);
    dmr_raw *raw = dmr_raw_new(len);
    DMR_ERROR_IF_NULL(raw, DMR_ENOMEM);
    byte_copy(raw->buf, buf, len);
    raw->len = len;
    return dmr_rawq_add(rawq, raw);
}

DMR_API int dmr_rawq_each(dmr_rawq *rawq, dmr_raw_cb cb, void *userdata)
{
    DMR_ERROR_IF_NULL(rawq, DMR_EINVAL);
    dmr_raw *raw, *next;
    DMR_TAILQ_FOREACH_SAFE(raw, &rawq->head, entries, next) {
        cb(raw, userdata);
    }
    return 0;
}

DMR_API bool dmr_rawq_empty(dmr_rawq *rawq)
{
    return rawq == NULL || DMR_TAILQ_EMPTY(&rawq->head);
}

DMR_API size_t dmr_rawq_size(dmr_rawq *rawq)
{
    if (rawq == NULL)
        return -1;

    size_t i = 0;
    dmr_raw *raw, *next;
    DMR_TAILQ_FOREACH_SAFE(raw, &rawq->head, entries, next) {
        i++;
    }
    return i;
}

DMR_API dmr_raw *dmr_rawq_shift(dmr_rawq *rawq)
{
    if (rawq == NULL)
        return NULL;

    dmr_raw *raw = DMR_TAILQ_FIRST(&rawq->head);
    if (raw != NULL)
        DMR_TAILQ_REMOVE(&rawq->head, raw, entries);

    return raw;
}

DMR_API int dmr_rawq_unshift(dmr_rawq *rawq, dmr_raw *raw)
{
    DMR_ERROR_IF_NULL(rawq, DMR_EINVAL);
    DMR_ERROR_IF_NULL(raw, DMR_EINVAL);

    if (rawq->limit == 0 || dmr_rawq_size(rawq) < rawq->limit) {
        DMR_TAILQ_INSERT_HEAD(&rawq->head, raw, entries);
        return 0;
    }

    /* queue full */
    return -1;
}
