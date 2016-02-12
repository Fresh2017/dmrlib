#ifndef _DMR_PROTOCOL_H
#define _DMR_PROTOCOL_H

#include <dmr.h>

#if defined(__cplusplus)
extern "C" {
#endif

typedef enum {
    DMR_PROTOCOL_UNKNOWN = 0,
    DMR_PROTOCOL_HOMEBREW,
    DMR_PROTOCOL_MBE,
    DMR_PROTOCOL_MMDVM
} dmr_protocol_type;

typedef struct dmr_protocol dmr_protocol;

#include <dmr/io.h>

typedef int (*dmr_init_io_cb)(dmr_io *io, void *instance);
typedef int (*dmr_register_io_cb)(dmr_io *io, void *instance);
typedef int (*dmr_close_io_cb)(dmr_io *io, void *instance);

struct dmr_protocol {
    dmr_protocol_type  type;
    char               *name;
    void               *instance;
    dmr_init_io_cb     init_io;
    dmr_register_io_cb register_io;
    dmr_close_io_cb    close_io;
};

#if defined(__cplusplus)
}
#endif

#endif // _DMR_PROTOCOL_H
