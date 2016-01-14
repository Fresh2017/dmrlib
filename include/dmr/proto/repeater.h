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

/**
 * Repeater router callback function.
 * @param repeater Repeater instance pointer.
 * @param route    Route instance pointer.
 */
typedef bool (*dmr_repeater_route_t)(dmr_repeater_t *, dmr_proto_t *, dmr_proto_t *, dmr_packet_t *);

typedef struct dmr_repeater_slot_s {
    void        *userdata;
    dmr_proto_t *proto;
} dmr_repeater_slot_t;

typedef struct dmr_repeater_timeslot_s {
    dmr_data_type_t   last_data_type;
    dmr_id_t          src_id, dst_id;
    bool              voice_call_active;
    uint8_t           voice_frame;
    bool              data_call_active;
    dmr_vbptc_16_11_t *vbptc_emb_lc;
} dmr_repeater_timeslot_t;

struct dmr_repeater_s {
    dmr_proto_t             proto;
    dmr_repeater_route_t    route;
    dmr_repeater_slot_t     slot[DMR_REPEATER_MAX_SLOTS];
    size_t                  slots;
    dmr_repeater_timeslot_t ts[2];
    dmr_color_code_t        color_code;
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

#endif // _DMR_PROTO_REPEATER_H
