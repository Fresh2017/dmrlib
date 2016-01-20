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
    dmr_log_mutex("repeater: mutex lock");
    dmr_mutex_lock(repeater->proto.mutex);
    repeater->proto.is_active = true;
    dmr_log_mutex("repeater: mutex unlock");
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
    dmr_log_mutex("repeater: active");
    dmr_repeater_t *repeater = (dmr_repeater_t *)repeaterptr;
    if (repeater == NULL)
        return false;

    dmr_log_mutex("repeater: mutex lock");
    dmr_mutex_lock(repeater->proto.mutex);
    bool active = repeater->proto.thread != NULL && repeater->proto.is_active;
    dmr_log_mutex("repeater: mutex unlock");
    dmr_mutex_unlock(repeater->proto.mutex);
    dmr_log_mutex("repeater: active = %s", DMR_LOG_BOOL(active));
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

static void repeater_proto_rx_cb(dmr_proto_t *proto, void *userdata, dmr_packet_t *packet_in)
{
    dmr_log_trace("repeater: rx callback %p", userdata);
    dmr_repeater_t *repeater = (dmr_repeater_t *)userdata;
    if (proto == NULL || repeater == NULL || packet_in == NULL)
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
            if (repeater->route(repeater, proto, slot->proto, packet_in)) {
                dmr_log_debug("repeater: routing %s packet from %s->%s",
                    dmr_data_type_name(packet_in->data_type),
                    proto->name, slot->proto->name);

                dmr_packet_t *packet = talloc(NULL, dmr_packet_t);
                if (packet == NULL) {
                    dmr_log_error("repeater: no memory to clone packet!");
                    return;
                }
                memcpy(packet, packet_in, sizeof(dmr_packet_t));

                uint8_t sequence = packet->meta.sequence;
                if (strcmp(slot->proto->name, "mmdvm")) {
                    dmr_repeater_fix_headers(repeater, packet);
                    dmr_log_debug("repeater: updated sequence %u->%u",
                        sequence, packet->meta.sequence);
                    packet->meta.sequence = sequence; // FIXME
                }
                dmr_proto_tx(slot->proto, slot->userdata, packet);
                talloc_free(packet);
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
    repeater->color_code = 1;
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
        dmr_msleep(25);
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

static int dmr_repeater_voice_call_end(dmr_repeater_t *repeater, dmr_packet_t *packet)
{
    if (repeater == NULL || packet == NULL)
        return dmr_error(DMR_EINVAL);

    dmr_ts_t ts = packet->ts;
    dmr_repeater_timeslot_t rts = repeater->ts[ts];

    if (!rts.voice_call_active)
        return 0;

    dmr_log_trace("repeater: voice call end on %s", dmr_ts_name(ts));
    rts.voice_call_active = false;

    if (rts.vbptc_emb_lc != NULL) {
        dmr_vbptc_16_11_free(rts.vbptc_emb_lc);
    }

    return 0;
}

static int dmr_repeater_voice_call_start(dmr_repeater_t *repeater, dmr_packet_t *packet, dmr_full_lc_t *full_lc)
{
    if (repeater == NULL || packet == NULL)
        return dmr_error(DMR_EINVAL);

    dmr_ts_t ts = packet->ts;
    dmr_repeater_timeslot_t rts = repeater->ts[ts];

    if (rts.voice_call_active) {
        return dmr_repeater_voice_call_end(repeater, packet);
    }    

    dmr_log_trace("repeater: voice call start on %s", dmr_ts_name(ts));
    rts.voice_call_active = true;
    rts.stream_id = packet->meta.stream_id;

    if (full_lc != NULL) {
        dmr_log_trace("repeater: constructing emb LC");
        if ((rts.vbptc_emb_lc = dmr_vbptc_16_11_new(8, NULL)) == NULL) {
            return dmr_error(DMR_ENOMEM);
        }

        dmr_emb_signalling_lc_bits_t *ebits = talloc(NULL, dmr_emb_signalling_lc_bits_t);
        dmr_emb_signalling_lc_bits_t *ibits;
        if (ebits == NULL)
            return dmr_error(DMR_ENOMEM);

        if (dmr_emb_encode_signalling_lc_from_full_lc(full_lc, ebits, packet->data_type) != 0) {
            talloc_free(ebits);
            return dmr_error(DMR_LASTERROR);
        }
        if ((ibits = dmr_emb_signalling_lc_interlave(ebits)) == NULL) {
            talloc_free(ebits);
            return dmr_error(DMR_ENOMEM);
        }
        if (dmr_vbptc_16_11_encode(rts.vbptc_emb_lc, ibits->bits, sizeof(dmr_emb_signalling_lc_bits_t)) != 0) {
            talloc_free(ebits);
            return dmr_error(DMR_LASTERROR);
        }
        talloc_free(ebits);
    }

    dmr_log_trace("repeater: reset sequence on %s", dmr_ts_name(ts));
    rts.sequence = 0;

    return 0;
}

int dmr_repeater_fix_headers(dmr_repeater_t *repeater, dmr_packet_t *packet)
{
    packet->ts = DMR_TS1;
    dmr_ts_t ts = packet->ts;
    dmr_repeater_timeslot_t rts = repeater->ts[ts];
    dmr_log_trace("repeater[%u]: fixed headers in %s packet",
        ts, dmr_data_type_name(packet->data_type));

    if (repeater == NULL || packet == NULL)
        return dmr_error(DMR_EINVAL);

    if ((rts.voice_call_active || rts.data_call_active) && packet->meta.stream_id != rts.stream_id) {
        dmr_log_debug("repeater[%u]: ignored stream id 0x%08x: call in progress", ts, packet->meta.stream_id);
        return 0;
    }

    if (packet->color_code != repeater->color_code) {
        dmr_log_debug("repeater[%u]: setting color code %u->%u",
            ts, packet->color_code, repeater->color_code);
        packet->color_code = repeater->color_code;
    }

    switch (packet->data_type) {
    case DMR_DATA_TYPE_VOICE_LC:
        dmr_log_trace("repeater[%u]: constructing Full Link Control", ts);

        // Regenerate full LC
        dmr_full_lc_t *full_lc = talloc(NULL, dmr_full_lc_t);
        if (full_lc == NULL) {
             dmr_log_error("repeater[%u]: out of memory", ts);
             return -1;
        }
        memset(full_lc, 0, sizeof(full_lc));
        full_lc->flco_pdu = (packet->flco == DMR_FLCO_PRIVATE) ? DMR_FLCO_PDU_PRIVATE : DMR_FLCO_PDU_GROUP;
        full_lc->privacy = (packet->flco == DMR_FLCO_PRIVATE);
        full_lc->src_id = packet->src_id;
        full_lc->dst_id = packet->dst_id;

        if (dmr_repeater_voice_call_start(repeater, packet, full_lc) != 0) {
            dmr_log_error("repeater[%u]: failed to start voice call: %s", ts, dmr_error_get());
            return -1;
        }

        if (dmr_full_lc_encode(full_lc, packet) != 0) {
            dmr_log_error("repeater[%u]: can't fix headers, full LC failed: ", ts, dmr_error_get());
            return -1;
        }


        dmr_log_trace("repeater[%u]: constructing sync pattern for voice LC", ts);
        dmr_sync_pattern_encode(DMR_SYNC_PATTERN_BS_SOURCED_DATA, packet);

        /*
        dmr_log_trace("repeater[%u]: setting slot type to voice LC", ts);
        dmr_slot_type_encode(packet);
        */

        // Override sequence
        packet->meta.sequence = (rts.sequence++) & 0xff;
        break;

    case DMR_DATA_TYPE_TERMINATOR_WITH_LC:
        dmr_sync_pattern_encode(DMR_SYNC_PATTERN_BS_SOURCED_DATA, packet);
        if (dmr_repeater_voice_call_end(repeater, packet) != 0) {
            dmr_log_error("repeater[%u]: failed to end voice call: %s", ts, dmr_error_get());
            return -1;
        }
        
        // Override sequence
        packet->meta.sequence = (rts.sequence++) & 0xff;
        break;

    case DMR_DATA_TYPE_VOICE_SYNC:
    case DMR_DATA_TYPE_VOICE:
        dmr_log_trace("repeater[%u]: setting SYNC data", ts);
        dmr_emb_signalling_lc_bits_t emb_bits;
        memset(&emb_bits, 0, sizeof(emb_bits));
        int ret = 0;

        dmr_emb_t emb = {
            .color_code = repeater->color_code,
            .pi         = packet->flco == DMR_FLCO_PRIVATE,
            .lcss       = DMR_EMB_LCSS_SINGLE_FRAGMENT
        };

        switch (packet->meta.voice_frame + 'A') {
        case 'A':
            /*
            // It may be late entry
            if (!rts.voice_call_active) {
                dmr_repeater_voice_call_start(repeater, packet, NULL);
            }
            */
            dmr_log_trace("repeater[%u]: constructing sync pattern for voice SYNC in frame A", ts);
            ret = dmr_sync_pattern_encode(DMR_SYNC_PATTERN_BS_SOURCED_VOICE, packet);
            break;

        case 'B':
            if (rts.vbptc_emb_lc == NULL) {
                dmr_log_trace("repeater[%u]: inserting NULL EMB LCSS fragment in frame B", ts);
            } else {
                dmr_log_trace("repeater[%u]: inserting first LCSS fragment in frame B", ts);
                emb.lcss = DMR_EMB_LCSS_FIRST_FRAGMENT;
            }
            ret = dmr_emb_lcss_fragment_encode(&emb, rts.vbptc_emb_lc, 0, packet);
            break;

        case 'C':
            if (rts.vbptc_emb_lc == NULL) {
                dmr_log_trace("repeater[%u]: inserting NULL EMB LCSS fragment in frame C", ts);
            } else {
                dmr_log_trace("repeater[%u: inserting continuation LCSS fragment in frame C", ts);
                emb.lcss = DMR_EMB_LCSS_CONTINUATION;
            }
            ret = dmr_emb_lcss_fragment_encode(&emb, rts.vbptc_emb_lc, 1, packet);
            break;

        case 'D':
            if (rts.vbptc_emb_lc == NULL) {
                dmr_log_trace("repeater[%u]: inserting NULL EMB LCSS fragment in frame D", ts);
            } else {
                dmr_log_trace("repeater[%u]: inserting continuation LCSS fragment in frame D", ts);
                emb.lcss = DMR_EMB_LCSS_CONTINUATION;
            }
            ret = dmr_emb_lcss_fragment_encode(&emb, rts.vbptc_emb_lc, 2, packet);
            break;

        case 'E':
            if (rts.vbptc_emb_lc == NULL) {
                dmr_log_trace("repeater[%u]: inserting NULL EMB LCSS fragment in frame E", ts);
            } else {
                dmr_log_trace("repeater[%u]: inserting last LCSS fragment in frame E", ts);
                emb.lcss = DMR_EMB_LCSS_LAST_FRAGMENT;
            }
            ret = dmr_emb_lcss_fragment_encode(&emb, rts.vbptc_emb_lc, 3, packet);
            break;

        case 'F':
            dmr_log_trace("repeater[%u]: inserting NULL EMB LCSS fragment in frame F", ts);
            ret = dmr_emb_lcss_fragment_encode(&emb, NULL, 0, packet);
            break;

        default:
            break;
        }

        if (ret != 0) {
            dmr_log_error("repeater[%u]: error setting SYNC data: %s", ts, dmr_error_get());
            return ret;
        }

        // Override sequence
        packet->meta.sequence = (rts.sequence++) & 0xff;
        break;

    default:
        dmr_log_trace("repeater[%u]: not altering %s packet", ts, dmr_data_type_name(packet->data_type));
        break;
    }

    return 0;
}

void dmr_repeater_free(dmr_repeater_t *repeater)
{
    if (repeater == NULL)
        return;

    free(repeater);
}
