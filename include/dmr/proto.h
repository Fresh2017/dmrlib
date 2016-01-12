/**
 * @file
 */
#ifndef _DMR_PROTO_H
#define _DMR_PROTO_H

#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>

#include <dmr/error.h>
#include <dmr/config.h>
#include <dmr/platform.h>
#include <dmr/type.h>
#include <dmr/thread.h>
#include <dmr/packet.h>
#include <dmr/payload/voice.h>

#define DMR_PROTO_CB_MAX 64

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
typedef int (*dmr_proto_init_t)(void *);
/** Protocol start function, starts the protocol handler thread. */
typedef int (*dmr_proto_start_t)(void *);
/** Protocol stop function, stops the protocol hander thread. */
typedef int (*dmr_proto_stop_t)(void *);
/** Protocol wait function, waits for the protocol handler thread to finish. */
typedef int (*dmr_proto_wait_t)(void *);
/** Check if the protocol handler thread is running. */
typedef bool (*dmr_proto_active_t)(void *);
/** Process data frame callback. */
typedef void *(*dmr_proto_process_data_t)(void *, uint8_t *, size_t);
/** Process voice frame callback.
 * @param ptr      Pointer to the protocol.
 * @param voice    Voice frame.
 * @param samples  Pointer to the samples buffer.
 */
typedef void (*dmr_proto_process_voice_t)(void *, dmr_voice_frame_t *, float *);
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
typedef void (*dmr_proto_io_cb_func_t)(dmr_proto_t *, void *, dmr_packet_t *);

typedef struct {
    dmr_proto_io_cb_func_t cb;
    void                   *userdata;
} dmr_proto_io_cb_t;

struct dmr_proto_s {
    const char                *name;
    dmr_proto_type_t          type;
    dmr_proto_init_t          init;
    bool                      init_done;
    dmr_proto_start_t         start;
    dmr_proto_stop_t          stop;
    dmr_proto_wait_t          wait;
    dmr_proto_active_t        active;
    dmr_proto_process_data_t  process_data;
    dmr_proto_process_voice_t process_voice;
    dmr_proto_io_t            tx;
    dmr_proto_io_cb_t         *tx_cb[DMR_PROTO_CB_MAX];
    size_t                    tx_cbs;
    dmr_proto_io_t            rx;
    dmr_proto_io_cb_t         *rx_cb[DMR_PROTO_CB_MAX];
    size_t                    rx_cbs;
    dmr_thread_t              *thread;
    bool                      is_active;
    dmr_mutex_t               *mutex;
};

//#define DMR_PROTO_INIT(p)     (p->proto.init(p))
//#define DMR_PROTO_START(p)    (p->proto.start(p))

extern int dmr_proto_init(void);
extern int dmr_proto_mutex_init(dmr_proto_t *proto);
extern int dmr_proto_call_init(void *mem);
extern int dmr_proto_call_start(void *mem);
extern int dmr_proto_call_stop(void *mem);
extern int dmr_proto_call_wait(void *mem);
extern bool dmr_proto_is_active(void *mem);
extern void dmr_proto_rx(dmr_proto_t *proto, void *userdata, dmr_packet_t *packet);
extern void dmr_proto_rx_cb_run(dmr_proto_t *proto, dmr_packet_t *packet);
extern bool dmr_proto_rx_cb_add(dmr_proto_t *proto, dmr_proto_io_cb_func_t cb, void *userdata);
extern bool dmr_proto_rx_cb_del(dmr_proto_t *proto, dmr_proto_io_cb_func_t cb);
extern void dmr_proto_tx(dmr_proto_t *proto, void *userdata, dmr_packet_t *packet);

#endif // _DMR_PROTO_H
