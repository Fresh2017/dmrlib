/**
 * @file
 */
#ifndef _DMR_PROTO_H
#define _DMR_PROTO_H

#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>

#include <dmr/platform.h>
#include <dmr/type.h>
#include <dmr/packet.h>
#include <dmr/payload/voice.h>

#define DMR_PROTO_CB_MAX 64

typedef enum {
    DMR_PROTO_OK,        /* Protocol initialized ok. */
    DMR_PROTO_MEM,       /* Out of memory. */
    DMR_PROTO_CONF,      /* Protocol needs configuration. */
    DMR_PROTO_NOT_READY, /* Protocol not ready. */
    DMR_PROTO_ERROR      /* Protocol generic error. */
} dmr_proto_status_t;

typedef enum {
    DMR_PROTO_UNKNOWN,
    DMR_PROTO_HOMEBREW,
    DMR_PROTO_MBE,
    DMR_PROTO_MMDVM,
    DMR_PROTO_REPEATER
} dmr_proto_type_t;

struct dmr_proto_s;
typedef struct dmr_proto_s dmr_proto_t;

/** Protocol initializer funciton */
typedef dmr_proto_status_t (*dmr_proto_init_t)(void *);
/** Protocol start function, starts the protocol handler thread. */
typedef bool (*dmr_proto_start_t)(void *);
/** Protocol stop function, stops the protocol hander thread. */
typedef bool (*dmr_proto_stop_t)(void *);
/** Check if the protocol handler thread is running. */
typedef bool (*dmr_proto_active_t)(void *);
/** Process data frame callback. */
typedef void *(*dmr_proto_process_data_t)(void *, uint8_t *, size_t);
/** Process voice frame callback.
 * @param ptr      Pointer to the protocol.
 * @param voice    Voice frame.
 * @param samples  Pointer to the samples buffer.
 */
typedef void (*dmr_proto_process_voice_t)(void *, dmr_voice_t *, float *);
/** Receive or transmit a raw DMR frame using the protocol.
 * @param ptr      Pointer to the protocol instance.
 * @param packet   Pointer to the DMR packet.
 */
typedef void (*dmr_proto_io_t)(void *, dmr_packet_t *);
/** Receive or transmit callback.
 * @param proto    Pointer to the protocol.
 * @param ptr      Pointer to the protocol instance.
 * @param packet   Pointer to the DMR packet.
 */
typedef void (*dmr_proto_io_cb_t)(dmr_proto_t *, void *, dmr_packet_t *);

struct dmr_proto_s {
    const char                *name;
    dmr_proto_type_t          type;
    dmr_proto_init_t          init;
    dmr_proto_start_t         start;
    dmr_proto_stop_t          stop;
    dmr_proto_active_t        active;
    dmr_proto_process_data_t  process_data;
    dmr_proto_process_voice_t process_voice;
    dmr_proto_io_t            tx;
    dmr_proto_io_cb_t         **tx_cb;
    size_t                    tx_cbs;
    dmr_proto_io_t            rx;
    dmr_proto_io_cb_t         **rx_cb;
    size_t                    rx_cbs;
};

extern void dmr_proto_rx(dmr_proto_t *proto, void *mem, dmr_packet_t *packet);
extern bool dmr_proto_rx_cb_add(dmr_proto_t *proto, dmr_proto_io_cb_t cb);
extern bool dmr_proto_rx_cb_del(dmr_proto_t *proto, dmr_proto_io_cb_t cb);

#endif // _DMR_PROTO_H
