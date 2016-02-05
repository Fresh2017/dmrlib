#ifndef _DMR_PROTOCOL_H
#define _DMR_PROTOCOL_H

#include <dmr/io.h>

#if defined(__cplusplus)
extern "C" {
#endif

typedef enum {
    DMR_PROTOCOL_UNKNOWN = 0,
    DMR_PROTOCOL_MMDVM
} dmr_protocol_type;

typedef struct {
    dmr_protocol_type type;
    char              *name;
    void              *instance;
    dmr_read_cb       readable;
    dmr_write_cb      writable;
    dmr_close_cb      closable;
} dmr_protocol;

extern dmr_protocol * dmr_protocol_init(dmr_protocol template, void *instance);

#if defined(__cplusplus)
}
#endif

#endif // _DMR_PROTOCOL_H
