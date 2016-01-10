#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "dmr/error.h"
#include "dmr/log.h"
#include "dmr/proto/repeater.h"

const char *dmr_repeater_proto_name = "dmrlib repeater";

static dmr_proto_status_t repeater_proto_init(void *repeaterptr)
{
    dmr_log_debug("repeater: init");

    dmr_repeater_t *repeater = (dmr_repeater_t *)repeaterptr;
    if (repeater == NULL)
        return DMR_PROTO_ERROR;

    if (repeater->slots < 2)
        return DMR_PROTO_CONF;

    return DMR_PROTO_OK;
}

static bool repeater_proto_start(void *repeaterptr)
{
    dmr_log_debug("repeater: start");
    return true;
}

static void repeater_proto_rx_cb(dmr_proto_t *proto, void *mem, dmr_packet_t *packet)
{

}

dmr_repeater_t *dmr_repeater_new(void)
{
    dmr_repeater_t *repeater = malloc(sizeof(dmr_repeater_t));
    if (repeater == NULL)
        return NULL;
    memset(repeater, 0, sizeof(dmr_repeater_t));

    // Setup repeater protocol
    repeater->proto.name = dmr_repeater_proto_name;
    repeater->proto.type = DMR_PROTO_REPEATER;
    repeater->proto.init = repeater_proto_init;
    repeater->proto.start = repeater_proto_start;
    repeater->slots = 0;

    return repeater;
}

void dmr_repeater_free(dmr_repeater_t *repeater)
{
    if (repeater == NULL)
        return;

    free(repeater);
}

bool dmr_repeater_add(dmr_repeater_t *repeater, void *ptr, dmr_proto_t *proto)
{
    if (repeater == NULL || ptr == NULL || proto == NULL)
        return false;

    if (repeater->slots >= DMR_REPEATER_MAX_SLOTS) {
        dmr_log_error("repeater: max slots of %d reached", DMR_REPEATER_MAX_SLOTS);
        return false;
    }

    // Register a calback for the repeater on the protocol.
    if (!dmr_proto_rx_cb_add(proto, repeater_proto_rx_cb)) {
        dmr_log_error("repeater: protocol %s callback refused", proto->name);
        return false;
    }

    repeater->slot[repeater->slots].ptr = ptr;
    repeater->slot[repeater->slots].proto = proto;
    repeater->slots++;
    dmr_log_info("repeater: added protocol %s", proto->name);
    return true;
}
