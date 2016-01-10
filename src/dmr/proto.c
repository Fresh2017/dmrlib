#include <stdlib.h>
#include "dmr/proto.h"

bool dmr_proto_init(void *mem)
{
    dmr_proto_t *proto = (dmr_proto_t *)mem;
    return proto->init(mem);
}

bool dmr_proto_start(void *mem)
{
    dmr_proto_t *proto = (dmr_proto_t *)mem;
    return proto->start(mem);
}

bool dmr_proto_stop(void *mem)
{
    dmr_proto_t *proto = (dmr_proto_t *)mem;
    return proto->stop(mem);
}

bool dmr_proto_active(void *mem)
{
    dmr_proto_t *proto = (dmr_proto_t *)mem;
    return proto->active(mem);
}

void dmr_proto_rx(dmr_proto_t *proto, void *mem, dmr_packet_t *packet)
{
    size_t i;
    dmr_proto_io_cb_t cb;

    if (proto == NULL || packet == NULL)
        return;
    if (proto->rx_cbs == 0)
        return;

    for (i = 0; i < proto->rx_cbs; i++) {
        cb = *proto->rx_cb[i];
        cb(proto, mem, packet);
    }
}

bool dmr_proto_rx_cb_add(dmr_proto_t *proto, dmr_proto_io_cb_t cb)
{
    if (proto == NULL || cb == NULL)
        return false;

    if (proto->rx_cbs == DMR_PROTO_CB_MAX - 1)
        return false;

    if (proto->rx_cb == NULL) {
        proto->rx_cb = malloc(sizeof(dmr_proto_io_cb_t) * DMR_PROTO_CB_MAX);
        if (proto->rx_cb == NULL)
            return false;
    }

    proto->rx_cb[proto->rx_cbs++] = &cb;
    return true;
}

bool dmr_proto_rx_cb_del(dmr_proto_t *proto, dmr_proto_io_cb_t cb)
{
    size_t i;

    if (proto == NULL || cb == NULL)
        return false;

    if (proto->rx_cb == NULL || proto->rx_cbs == 0)
        return false;

    for (i = 0; i < proto->rx_cbs; i++) {
        if (proto->rx_cb[i] == &cb) {
            for (; i < proto->rx_cbs; i++) {
                proto->rx_cb[i] = proto->rx_cb[i + 1];
            }
            proto->rx_cb[i] = NULL;
            proto->rx_cbs--;
            return true;
        }
    }

    return false;
}
