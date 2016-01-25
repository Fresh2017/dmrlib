#include <stdlib.h>
#include "dmr/proto.h"

dmr_mutex_t *dmr_proto_lock;

int dmr_proto_init(void)
{
    if (dmr_proto_lock != NULL)
        return 0;

    dmr_proto_lock = talloc_zero(NULL, dmr_mutex_t);
    if (dmr_proto_lock == NULL)
        return dmr_error(DMR_ENOMEM);

    if (dmr_mutex_init(dmr_proto_lock, dmr_mutex_plain) != dmr_thread_success)
        return dmr_error(DMR_EINVAL);

    return 0;
}

int dmr_proto_mutex_init(dmr_proto_t *proto)
{
    if (proto == NULL)
        return dmr_error(DMR_EINVAL);

    if (proto->mutex != NULL)
        return 0;

    proto->mutex = talloc_zero(NULL, dmr_mutex_t);
    if (proto->mutex == NULL)
        return dmr_error(DMR_ENOMEM);

    if (dmr_mutex_init(proto->mutex, dmr_mutex_plain) != dmr_thread_success) {
        dmr_log_error("proto: failed to init mutex for %s", proto->name);
        return dmr_error(DMR_EINVAL);
    }
    return 0;
}

int dmr_proto_call_init(void *mem)
{
    dmr_log_trace("proto: init %p", mem);
    dmr_proto_t *proto = (dmr_proto_t *)mem;
    if (proto == NULL || proto->init == NULL)
        return dmr_error(DMR_EINVAL);
    return proto->init(mem);
}

int dmr_proto_call_start(void *mem)
{
    dmr_log_trace("proto: start %p", mem);
    dmr_proto_t *proto = (dmr_proto_t *)mem;
    if (proto == NULL || proto->start == NULL)
        return dmr_error(DMR_EINVAL);
    return proto->start(mem);
}

int dmr_proto_call_stop(void *mem)
{
    dmr_log_trace("proto: stop %p", mem);
    dmr_proto_t *proto = (dmr_proto_t *)mem;
    if (proto == NULL || proto->stop == NULL)
        return dmr_error(DMR_EINVAL);
    return proto->stop(mem);
}

int dmr_proto_call_wait(void *mem)
{
    dmr_log_trace("proto: wait %p", mem);
    dmr_proto_t *proto = (dmr_proto_t *)mem;
    if (proto == NULL || proto->wait == NULL)
        return dmr_error(DMR_EINVAL);
    return proto->wait(mem);
}

bool dmr_proto_is_active(void *mem)
{
    dmr_log_trace("proto: active %p", mem);
    dmr_proto_t *proto = (dmr_proto_t *)mem;
    if (proto == NULL || proto->active == NULL)
        return dmr_error(DMR_EINVAL);
    return proto->active(mem);
}

void dmr_proto_rx(dmr_proto_t *proto, void *userdata, dmr_packet_t *packet)
{
    dmr_log_trace("proto: rx %p", proto);

    if (proto == NULL)
        return;
    if (proto->rx == NULL) {
        dmr_log_error("proto: %s does not support rx", proto->name);
        return;
    }
    proto->rx(userdata, packet);
}

void dmr_proto_rx_cb_run(dmr_proto_t *proto, dmr_packet_t *packet)
{
    dmr_log_trace("proto: rx %s", proto->name);
    size_t i;

    if (proto == NULL || proto->rx_cbs == 0 || packet == NULL)
        return;

    for (i = 0; i < proto->rx_cbs; i++) {
        dmr_proto_io_cb_t *cb = proto->rx_cb[i];
        if (cb == NULL || cb->cb == NULL) {
            dmr_log_error("proto: rx callback %d is NULL", i);
            continue;
        }
        cb->cb(proto, cb->userdata, packet);
    }
}

bool dmr_proto_rx_cb_add(dmr_proto_t *proto, dmr_proto_io_cb_func_t cb, void *userdata)
{
    if (proto == NULL || cb == NULL)
        return false;

    if (proto->rx_cbs == DMR_PROTO_CB_MAX - 1)
        return false;

    if (proto->rx_cb[proto->rx_cbs] == NULL) {
        proto->rx_cb[proto->rx_cbs] = talloc_zero(NULL, dmr_proto_io_cb_t);
    }
    if (proto->rx_cb[proto->rx_cbs] == NULL) {
        dmr_error(DMR_ENOMEM);
        return false;
    }

    proto->rx_cb[proto->rx_cbs]->cb = cb;
    proto->rx_cb[proto->rx_cbs]->userdata = userdata;
    proto->rx_cbs++;
    return true;
}

bool dmr_proto_rx_cb_del(dmr_proto_t *proto, dmr_proto_io_cb_func_t cb)
{
    size_t i;

    if (proto == NULL || cb == NULL)
        return false;

    if (proto->rx_cbs == 0)
        return false;

    for (i = 0; i < proto->rx_cbs; i++) {
        if (proto->rx_cb[i]->cb == cb) {
            free(proto->rx_cb[i]);
            proto->rx_cb[i] = NULL;
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

void dmr_proto_tx(dmr_proto_t *proto, void *userdata, dmr_packet_t *packet)
{
    dmr_log_trace("proto: tx %p", proto);

    if (proto == NULL)
        return;
    if (proto->tx == NULL) {
        dmr_log_error("proto: %s does not support tx", proto->name);
        return;
    }
    proto->tx(userdata, packet);
}
