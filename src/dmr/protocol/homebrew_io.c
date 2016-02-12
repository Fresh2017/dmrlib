#include "dmr.h"
#include "dmr/protocol/homebrew.h"
#include "dmr/io.h"
#include "dmr/malloc.h"
#include "common/socket.h"

DMR_PRV static int homebrew_io_ping_timer(dmr_io *io, void *homebrewptr);
DMR_PRV static int homebrew_io_readable(dmr_io *io, void *homebrewptr, int fd);
DMR_PRV static int homebrew_io_error(dmr_io *io, void *homebrewptr, int fd);
DMR_PRV static int homebrew_io_close(dmr_io *io, void *homebrewptr);

DMR_PRV static int homebrew_io_init(dmr_io *io, void *homebrewptr)
{
    DMR_ERROR_IF_NULL(io, DMR_EINVAL);
    DMR_ERROR_IF_NULL(homebrewptr, DMR_EINVAL);

    dmr_log_debug("homebrew io: init io");

    dmr_homebrew *homebrew = (dmr_homebrew *)homebrewptr;
    socket_t *sock = (socket_t *)homebrew->sock;
    if (sock == NULL) {
        dmr_log_critical("homebrew io: can't initialize for I/O loop, protocol not setup");
        return dmr_error(DMR_EINVAL);
    }

    /* Setup queues */
    DMR_ERROR_IF_NULL(homebrew->rxq = dmr_packetq_new(), DMR_ENOMEM);
    DMR_ERROR_IF_NULL(homebrew->txq = dmr_packetq_new(), DMR_ENOMEM);
    DMR_ERROR_IF_NULL(homebrew->rrq = dmr_rawq_new(32), DMR_ENOMEM);
    DMR_ERROR_IF_NULL(homebrew->trq = dmr_rawq_new(32), DMR_ENOMEM);

    return 0;
}

DMR_PRV int homebrew_io_register(dmr_io *io, void *homebrewptr)
{
    DMR_ERROR_IF_NULL(io, DMR_EINVAL);
    DMR_ERROR_IF_NULL(homebrewptr, DMR_EINVAL);

    dmr_log_debug("homebrew io: register io");

    dmr_homebrew *homebrew = (dmr_homebrew *)homebrewptr;
    socket_t *sock = (socket_t *)homebrew->sock;
    
    /* ping the repeater every 5 seconds */
    struct timeval ping_timer = { 5, 0 };

    /* register events, no write event, because datagram sockets are always writable */
    dmr_io_reg_timer(io, ping_timer, homebrew_io_ping_timer, homebrew, false);
    dmr_io_reg_read (io, sock->fd,   homebrew_io_readable,   homebrew, false);
    dmr_io_reg_error(io, sock->fd,   homebrew_io_error,      homebrew, false);
    dmr_io_reg_close(io,             homebrew_io_close,      homebrew       );

    return 0;
}

/* I/O callbacks */

DMR_PRV static int homebrew_io_ping_timer(dmr_io *io, void *homebrewptr)
{
    DMR_ERROR_IF_NULL(io, DMR_EINVAL);
    DMR_ERROR_IF_NULL(homebrewptr, DMR_EINVAL);

    dmr_homebrew *homebrew = (dmr_homebrew *)homebrewptr;

    /* check if we're logged in */
    if (homebrew->state != DMR_HOMEBREW_AUTH_DONE)
        return 0;

    /* check what time we received the last ping reply */
    if (dmr_time_since(homebrew->last_pong) > 10) {
        dmr_log_error("homebrew io: repeater ping timeout");
        /* request closing from the I/O loop */
        return -1;
    }

    dmr_log_debug("homebrew io: repeater ping");
    dmr_raw *raw = dmr_raw_new(15);
    DMR_ERROR_IF_NULL(raw, DMR_ENOMEM);
    dmr_raw_add(raw, "MSTPING", 7);
    dmr_raw_add_xuint32(raw, homebrew->config.repeater_id);
    dmr_homebrew_send_raw(homebrew, raw);
    gettimeofday(&homebrew->last_ping, NULL);

    return 0;
}

DMR_PRV static int homebrew_io_readable(dmr_io *io, void *homebrewptr, int fd)
{
    DMR_UNUSED(io);
    DMR_UNUSED(fd);
    DMR_ERROR_IF_NULL(homebrewptr, DMR_EINVAL);

    dmr_homebrew *homebrew = (dmr_homebrew *)homebrewptr;
    dmr_parsed_packet **parsed;
    DMR_ERROR_IF_NULL(parsed = dmr_malloc(dmr_parsed_packet *), DMR_ENOMEM);

    int ret = dmr_homebrew_read(homebrew, parsed);
    if (ret == 0 && *parsed != NULL) {
        dmr_log_debug("homebrew io: queued parsed packet");
        ret = dmr_packetq_add(homebrew->rxq, *parsed);
    } else {
        dmr_log_debug("homebrew io: no parsed packet");
    }

    dmr_free(parsed);
    return ret;
}

DMR_PRV static int homebrew_io_error(dmr_io *io, void *homebrewptr, int fd)
{
    DMR_UNUSED(fd);
    DMR_ERROR_IF_NULL(io, DMR_EINVAL);
    DMR_ERROR_IF_NULL(homebrewptr, DMR_EINVAL);

    dmr_homebrew *homebrew = (dmr_homebrew *)homebrewptr;
    dmr_log_critical("homebrew io: socket error");
    dmr_free(homebrew);
    return 0;
}

DMR_PRV static int homebrew_io_close(dmr_io *io, void *homebrewptr)
{
    DMR_ERROR_IF_NULL(io, DMR_EINVAL);
    DMR_ERROR_IF_NULL(homebrewptr, DMR_EINVAL);

    dmr_homebrew *homebrew = (dmr_homebrew *)homebrewptr;
    socket_t *sock = (socket_t *)homebrew->sock;
    if (sock == NULL)
        return 0; /* nothing to do */

    return dmr_homebrew_close(homebrew);
}

DMR_API dmr_protocol dmr_homebrew_protocol = {
    .type        = DMR_PROTOCOL_HOMEBREW,
    .name        = "Homebrew IPSC",
    .init_io     = homebrew_io_init,
    .register_io = homebrew_io_register,
    .close_io    = homebrew_io_close,
};
