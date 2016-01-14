#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "dmr/bits.h"
#include "dmr/error.h"
#include "dmr/log.h"
#include "dmr/proto/repeater.h"
#include "dmr/payload/emb.h"
#include "dmr/payload/lc.h"
#include "dmr/payload/sync.h"

const char *dmr_repeater_proto_name = "dmrlib repeater";

static int repeater_proto_init(void *repeaterptr)
{
    dmr_log_debug("repeater: init %p", repeaterptr);

    dmr_repeater_t *repeater = (dmr_repeater_t *)repeaterptr;
    if (repeater == NULL) {
        return dmr_error(DMR_EINVAL);
    }

    if (repeater->slots < 2) {
        dmr_log_error("repeater: can't start with less than 2 slots, got %d",
            repeater->slots);
        return dmr_error(DMR_EINVAL);
    }
    if (repeater->color_code < 1 || repeater->color_code > 15) {
        dmr_log_error("repeater: can't start, invalid color code %d",
            repeater->color_code);
        return dmr_error(DMR_EINVAL);
    }

    return 0;
}

static int repeater_proto_start_thread(void *repeaterptr)
{
    dmr_log_debug("repeater: start thread %d", dmr_thread_id(NULL) % 1000);
    dmr_repeater_t *repeater = (dmr_repeater_t *)repeaterptr;
    if (repeater == NULL)
        return dmr_thread_error;

    dmr_thread_name_set("repeater proto");
    dmr_log_trace("repeater: mutex lock");
    dmr_mutex_lock(repeater->proto.mutex);
    repeater->proto.is_active = true;
    dmr_log_trace("repeater: mutex unlock");
    dmr_mutex_unlock(repeater->proto.mutex);
    dmr_repeater_loop(repeater);
    dmr_thread_exit(dmr_thread_success);
    return dmr_thread_success;
}

static int repeater_proto_start(void *repeaterptr)
{
    dmr_log_debug("repeater: start %p", repeaterptr);
    dmr_repeater_t *repeater = (dmr_repeater_t *)repeaterptr;

    if (repeater == NULL)
        return dmr_error(DMR_EINVAL);

    if (repeater->proto.thread != NULL) {
        dmr_log_error("repeater: can't start, already active");
        return dmr_error(DMR_EINVAL);
    }

    dmr_mutex_lock(repeater->proto.mutex);
    repeater->proto.is_active = false;
    dmr_mutex_unlock(repeater->proto.mutex);
    repeater->proto.thread = malloc(sizeof(dmr_thread_t));
    if (repeater->proto.thread == NULL) {
        return dmr_error(DMR_ENOMEM);
    }

    if (dmr_thread_create(repeater->proto.thread, repeater_proto_start_thread, repeater) != dmr_thread_success) {
        dmr_log_error("repeater: can't create thread");
        return dmr_error(DMR_EINVAL);
    }
    return 0;
}

static bool repeater_proto_active(void *repeaterptr)
{
    dmr_log_trace("repeater: active");
    dmr_repeater_t *repeater = (dmr_repeater_t *)repeaterptr;
    if (repeater == NULL)
        return false;

    dmr_log_trace("repeater: mutex lock");
    dmr_mutex_lock(repeater->proto.mutex);
    bool active = repeater->proto.thread != NULL && repeater->proto.is_active;
    dmr_log_trace("repeater: mutex unlock");
    dmr_mutex_unlock(repeater->proto.mutex);
    dmr_log_trace("repeater: active = %s", DMR_LOG_BOOL(active));
    return active;
}

static int repeater_proto_stop(void *repeaterptr)
{
    dmr_log_debug("repeater: stop %p", repeaterptr);
    dmr_repeater_t *repeater = (dmr_repeater_t *)repeaterptr;
    if (repeater == NULL)
        return dmr_error(DMR_EINVAL);

    if (repeater->proto.thread == NULL) {
        fprintf(stderr, "repeater: not active\n");
        return 0;
    }

    dmr_mutex_lock(repeater->proto.mutex);
    repeater->proto.is_active = false;
    dmr_mutex_unlock(repeater->proto.mutex);
    if (dmr_thread_join(*repeater->proto.thread, NULL) != dmr_thread_success) {
        dmr_log_error("repeater: can't join thread");
        return dmr_error(DMR_EINVAL);
    }

    dmr_thread_exit(dmr_thread_success);
    free(repeater->proto.thread);
    repeater->proto.thread = NULL;
    return 0;
}

static int repeater_proto_wait(void *repeaterptr)
{
    dmr_log_trace("repeater: wait %p", repeaterptr);
    dmr_repeater_t *repeater = (dmr_repeater_t *)repeaterptr;
    if (repeater == NULL)
        return dmr_error(DMR_EINVAL);

    if (!repeater_proto_active(repeater))
        return 0;

    int ret;
    if ((ret = dmr_thread_join(*repeater->proto.thread, NULL)) != 0) {
        dmr_log_error("repeater: failed to join thread: %s", strerror(errno));
    }
    return 0;
}

static void repeater_proto_rx_cb(dmr_proto_t *proto, void *userdata, dmr_packet_t *packet)
{
    dmr_log_trace("repeater: rx callback %p", userdata);
    dmr_repeater_t *repeater = (dmr_repeater_t *)userdata;
    if (proto == NULL || repeater == NULL || packet == NULL)
        return;

    dmr_log_debug("repeater: rx callback from %s", proto->name);
    if (repeater->slots == 0) {
        dmr_log_error("repeater: no slots!?");
        return;
    }

    size_t i = 0;
    for (; i < repeater->slots; i++) {
        dmr_repeater_slot_t *slot = &repeater->slot[i];
        if (slot->proto == proto) {
            dmr_log_trace("repeater: skipped same-proto %s", slot->proto->name);
            continue;
        }
        dmr_log_debug("repeater: call route callback %p for %s->%s",
            repeater->route, proto->name, slot->proto->name);

        if (repeater->route != NULL) {
            if (repeater->route(repeater, proto, slot->proto, packet)) {
                dmr_log_debug("repeater: routing %s packet from %s->%s",
                    dmr_data_type_name(packet->data_type),
                    proto->name, slot->proto->name);
                dmr_repeater_fix_headers(repeater, packet);
                dmr_proto_tx(slot->proto, slot->userdata, packet);
            } else {
                dmr_log_debug("repeater: dropping packet, refused by router");
            }
        }
    }
}

dmr_repeater_t *dmr_repeater_new(dmr_repeater_route_t route)
{
    dmr_repeater_t *repeater = malloc(sizeof(dmr_repeater_t));
    if (repeater == NULL)
        return NULL;
    memset(repeater, 0, sizeof(dmr_repeater_t));
    repeater->route = route;

    if (repeater->route == NULL) {
        dmr_log_warn("repeater: got a NULL router, hope that's okay...");
    }

    // Setup repeater protocol
    repeater->proto.name = dmr_repeater_proto_name;
    repeater->proto.type = DMR_PROTO_REPEATER;
    repeater->proto.init = repeater_proto_init;
    repeater->proto.start = repeater_proto_start;
    repeater->proto.stop = repeater_proto_stop;
    repeater->proto.wait = repeater_proto_wait;
    repeater->proto.active = repeater_proto_active;
    repeater->slots = 0;
    if (dmr_proto_mutex_init(&repeater->proto) != 0) {
        dmr_log_error("repeater: failed to init mutex");
        free(repeater);
        return NULL;
    }

    return repeater;
}

void dmr_repeater_loop(dmr_repeater_t *repeater)
{
    // Run callbacks
    while (repeater_proto_active(repeater)) {
        dmr_sleep(1);
    }
}

int dmr_repeater_add(dmr_repeater_t *repeater, void *userdata, dmr_proto_t *proto)
{
    dmr_log_trace("repeater: add(%p, %p, %p)", repeater, userdata, proto);
    if (repeater == NULL || userdata == NULL || proto == NULL) {
        dmr_log_error("repeater: add received NULL pointer");
        return dmr_error(DMR_EINVAL);
    }

    if (repeater->slots >= DMR_REPEATER_MAX_SLOTS) {
        dmr_log_error("repeater: max slots of %d reached", DMR_REPEATER_MAX_SLOTS);
        return dmr_error(DMR_EINVAL);
    }

    // Register a calback for the repeater on the protocol.
    if (!dmr_proto_rx_cb_add(proto, repeater_proto_rx_cb, repeater)) {
        dmr_log_error("repeater: protocol %s callback refused", proto->name);
        return dmr_error(DMR_EINVAL);
    }

    repeater->slot[repeater->slots].userdata = userdata;
    repeater->slot[repeater->slots].proto = proto;
    repeater->slots++;
    dmr_log_info("repeater: added protocol %s", proto->name);
    return 0;
}

int dmr_repeater_fix_headers(dmr_repeater_t *repeater, dmr_packet_t *packet)
{
    dmr_log_trace("repeater: fixed headers in %s packet",
        dmr_data_type_name(packet->data_type));
    if (repeater == NULL || packet == NULL)
        return dmr_error(DMR_EINVAL);

    dmr_repeater_timeslot_t rts = repeater->ts[packet->ts];
    switch (packet->data_type) {
    case DMR_DATA_TYPE_VOICE_LC:
    case DMR_DATA_TYPE_VOICE_SYNC:
    case DMR_DATA_TYPE_VOICE:
    case DMR_DATA_TYPE_TERMINATOR_WITH_LC:
        switch (packet->data_type) {
        case DMR_DATA_TYPE_VOICE_LC:
            dmr_log_trace("repeater: constructing Full Link Control");

            // Regenerate full LC
            dmr_full_lc_t full_lc;
            memset(&full_lc, 0, sizeof(full_lc));
            DMR_UINT32_BE_PACK3(full_lc.fields.dst_id, packet->dst_id);
            DMR_UINT32_BE_PACK3(full_lc.fields.src_id, packet->src_id);
            if (packet->flco == DMR_FLCO_PRIVATE) {
                full_lc.fields.flco = 3;
            }
            if (dmr_full_lc_encode(&full_lc, packet) != 0) {
                dmr_log_error("repeater: can't fix headers, full LC failed");
                return -1;
            }
            dmr_log_trace("repeater: constructing sync pattern for voice LC");
            dmr_sync_pattern_encode(DMR_SYNC_PATTERN_BS_SOURCED_VOICE, packet);

            dmr_log_trace("repeater: setting slot type to voice LC");
            dmr_slot_type_encode(packet);

            dmr_log_trace("repeater: constructing emb LC");
            if (repeater->ts[packet->ts].vbptc_emb_lc != NULL) {
                dmr_vbptc_16_11_free(rts.vbptc_emb_lc);
            }
            if ((rts.vbptc_emb_lc = dmr_vbptc_16_11_new(8, repeater)) == NULL) {
                return dmr_error(DMR_ENOMEM);
            }

            dmr_emb_signalling_lc_bits_t *ebits = talloc(repeater, dmr_emb_signalling_lc_bits_t);
            dmr_emb_signalling_lc_bits_t *ibits;
            if (ebits == NULL)
                return dmr_error(DMR_ENOMEM);

            if (dmr_emb_encode_signalling_lc_from_full_lc(&full_lc, ebits) != 0)
                return dmr_error(DMR_LASTERROR);
            if ((ibits = dmr_emb_signalling_lc_interlave(ebits)) == NULL)
                return dmr_error(DMR_ENOMEM);
            if (dmr_vbptc_16_11_encode(rts.vbptc_emb_lc, ibits->bits, sizeof(dmr_emb_signalling_lc_bits_t)) != 0)
                return dmr_error(DMR_LASTERROR);

            break;

        case DMR_DATA_TYPE_TERMINATOR_WITH_LC:
            if (rts.vbptc_emb_lc != NULL) {
                talloc_free(rts.vbptc_emb_lc);
                rts.vbptc_emb_lc = NULL;
            }
            break;

        case DMR_DATA_TYPE_VOICE_SYNC:
        case DMR_DATA_TYPE_VOICE:
            dmr_log_trace("repeater: setting SYNC data");
            dmr_emb_signalling_lc_bits_t emb_bits;
            memset(&emb_bits, 0, sizeof(emb_bits));

            switch (packet->meta.voice_frame + 'A') {
            case 'A':
                dmr_log_trace("repeater: constructing sync pattern for voice SYNC");
                dmr_sync_pattern_encode(DMR_SYNC_PATTERN_BS_SOURCED_VOICE, packet);
                dmr_slot_type_encode(packet);
                break;

            case 'B':
                dmr_log_trace("repeater: inserting first LCSS fragment");
                dmr_emb_lcss_fragment_encode(DMR_EMB_LCSS_FIRST_FRAGMENT, rts.vbptc_emb_lc, 0, packet);
                dmr_slot_type_encode(packet);
                break;

            case 'C':
                dmr_log_trace("repeater: inserting continuation LCSS fragment");
                dmr_emb_lcss_fragment_encode(DMR_EMB_LCSS_CONTINUATION, rts.vbptc_emb_lc, 1, packet);
                dmr_slot_type_encode(packet);
                break;

            case 'D':
                dmr_log_trace("repeater: inserting continuation LCSS fragment");
                dmr_emb_lcss_fragment_encode(DMR_EMB_LCSS_CONTINUATION, rts.vbptc_emb_lc, 2, packet);
                dmr_slot_type_encode(packet);
                break;

            case 'E':
                dmr_log_trace("repeater: inserting last LCSS fragment");
                dmr_emb_lcss_fragment_encode(DMR_EMB_LCSS_LAST_FRAGMENT, rts.vbptc_emb_lc, 3, packet);
                dmr_slot_type_encode(packet);
                break;

            case 'F':
                dmr_log_trace("repeater: inserting NULL EMB LCSS fragment");
                dmr_emb_lcss_fragment_encode(DMR_EMB_LCSS_SINGLE_FRAGMENT, NULL, 0, packet);
                dmr_slot_type_encode(packet);
                break;

            default:
                break;
            }
            break;


            break;

        default:
            break;
        }
        break;

    default:
        break;
    }

    if (packet->data_type < DMR_DATA_TYPE_INVALID)
        dmr_slot_type_encode(packet);

    return 0;
}

void dmr_repeater_free(dmr_repeater_t *repeater)
{
    if (repeater == NULL)
        return;

    free(repeater);
}
