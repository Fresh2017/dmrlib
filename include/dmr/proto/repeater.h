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

struct dmr_repeater_s;
typedef struct dmr_repeater_s dmr_repeater_t;

/**
 * Repeater router callback function.
 * @param repeater Repeater instance pointer.
 * @param route    Route instance pointer.
 */
typedef bool (*dmr_repeater_route_t)(dmr_repeater_t *, dmr_proto_t *, dmr_packet_t *);

typedef struct dmr_repeater_slot_s {
    void        *ptr;
    dmr_proto_t *proto;
} dmr_repeater_slot_t;

struct dmr_repeater_s {
    dmr_proto_t          proto;
    dmr_repeater_route_t *route;
    dmr_repeater_slot_t  slot[DMR_REPEATER_MAX_SLOTS];
    size_t               slots;
};

/**
 * Create a new repeater.
 */
extern dmr_repeater_t *dmr_repeater_new(void);
/**
 * Deallocate repeater instance.
 * @param repeater Repeater instance pointer.
 */
extern void dmr_repeater_free(dmr_repeater_t *repeater);
/**
 * Add a protocol to the repeater.
 * @param repeater Repeater instance pointer.
 * @param ptr      Pointer to the protocol.
 * @param proto    Protocol instance pointer.
 */
extern bool dmr_repeater_add(dmr_repeater_t *repeater, void *ptr, dmr_proto_t *proto);
/**
 * Returns the number of active protocols on the repeater. If an error occurs,
 * -1 will be returned.
 * @param repeater Repeater instance pointer.
 */
extern int dmr_repeater_protos(dmr_repeater_t *repeater);

#endif // _DMR_PROTO_REPEATER_H
