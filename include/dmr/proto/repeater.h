/**
 * @file
 * @brief Repeater protocol suitable for linking other protocols.
 */
#ifndef _DMR_PROTO_REPEATER_H
#define _DMR_PROTO_REPEATER_H

#define DMR_REPEATER_MAX_SLOTS 64

#include <inttypes.h>

#include <dmr/proto.h>
#include <dmr/packet.h>
#include <dmr/fec/vbptc_16_11.h>

struct dmr_repeater_s;
typedef struct dmr_repeater_s dmr_repeater_t;

typedef enum {
    DMR_ROUTE_REJECT = 0x00,
    DMR_ROUTE_PERMIT,
    DMR_ROUTE_UNMODIFIED 
} dmr_route_t;

/**
 * Repeater router callback function.
 * @param repeater Repeater instance pointer.
 * @param route    Route instance pointer.
 */
typedef dmr_route_t (*dmr_repeater_route_t)(dmr_repeater_t *, dmr_proto_t *, dmr_proto_t *, dmr_packet_t *);

typedef struct dmr_repeater_slot_s {
    void        *userdata;
    dmr_proto_t *proto;
} dmr_repeater_slot_t;

typedef struct dmr_repeater_item_s {
    dmr_proto_t  *proto;
    dmr_packet_t *packet;
} dmr_repeater_item_t;

typedef struct dmr_repeater_timeslot_s {
    dmr_data_type_t   last_data_type;
    dmr_id_t          src_id, dst_id;
    uint32_t          stream_id;
    bool              voice_call_active;
    uint8_t           voice_frame;
    struct timeval    *last_voice_frame_received;
    bool              data_call_active;
    struct timeval    *last_data_frame_received;
    dmr_vbptc_16_11_t *vbptc_emb_lc;
    uint16_t          sequence;
    dmr_mutex_t       *lock;
} dmr_repeater_timeslot_t;

struct dmr_repeater_s {
    dmr_proto_t             proto;
    dmr_repeater_route_t    route;
    dmr_repeater_slot_t     slot[DMR_REPEATER_MAX_SLOTS];
    size_t                  slots;
    dmr_repeater_timeslot_t *ts;
    dmr_color_code_t        color_code;
    dmr_repeater_item_t     **queue;
    size_t                  queue_size;
    size_t                  queue_used;
    dmr_mutex_t             *queue_lock;
};

/** Create a new repeater. */
extern dmr_repeater_t *dmr_repeater_new(dmr_repeater_route_t);
/** Run repeater callback threads. */
extern void dmr_repeater_loop(dmr_repeater_t *repeater);
/** Add a protocol to the repeater.
 * @param repeater Repeater instance pointer.
 * @param ptr      Pointer to the protocol.
 * @param proto    Protocol instance pointer.
 */
extern int dmr_repeater_add(dmr_repeater_t *repeater, void *ptr, dmr_proto_t *proto);
/** Deallocate repeater instance.
 * @param repeater Repeater instance pointer.
 */
extern void dmr_repeater_free(dmr_repeater_t *repeater);
/** Returns the number of active protocols on the repeater. If an error occurs,
 * -1 will be returned.
 * @param repeater Repeater instance pointer.
 */
extern int dmr_repeater_protos(dmr_repeater_t *repeater);
/** Fix the EMB/LC headers in voice frames */
extern int dmr_repeater_fix_headers(dmr_repeater_t *repeater, dmr_packet_t *packet);

/** Add a packet to the processing queue. */
extern int dmr_repeater_queue(dmr_repeater_t *repeater, dmr_proto_t *proto, dmr_packet_t *packet);

#endif // _DMR_PROTO_REPEATER_H
