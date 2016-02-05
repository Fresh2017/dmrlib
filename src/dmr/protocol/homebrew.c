#include <string.h>
#include "dmr/error.h"
#include "dmr/malloc.h"
#include "dmr/protocol/homebrew.h"
#include "common/byte.h"
#include "common/buffer.h"
#include "common/format.h"
#include "common/scan.h"
#include "common/socket.h"

DMR_API dmr_homebrew *dmr_homebrew_new(char *peer_addr, uint16_t peer_port, char *bind_addr, uint16_t bind_port)
{
    if (peer_addr == NULL) {
        dmr_log_critical("homebrew: peer_addr can't be NULL");
        errno = EINVAL;
        return NULL;
    }

    /* Setup homebrew struct */
    DMR_MALLOC_CHECK(dmr_homebrew, homebrew);

    /* Setup addresses */
    if (peer_port == 0)
        peer_port = DMR_HOMEBREW_PORT;
    if (peer_addr != NULL && scan_ip6(peer_addr, homebrew->peer_ip) < 4) {
        dmr_log_critical("homebrew: invalid IP address \"%s\"", peer_addr);
        dmr_free(homebrew);
        return NULL;
    } 
    if (bind_addr != NULL && scan_ip6(bind_addr, homebrew->bind_ip) < 4) {
        dmr_log_critical("homebrew: invalid IP address \"%s\"", bind_addr);
        dmr_free(homebrew);
        return NULL;
    } else if (bind_addr == NULL) {
        byte_copy(homebrew->bind_ip, ip6any, 16);
    }

    /* Setup socket */
    socket_t *sock = socket_udp6(0);
    DMR_NULL_CHECK_FREE(sock, homebrew);

    /* Setup queues */
    DMR_NULL_CHECK_FREE(homebrew->rxq = dmr_packetq_new(), homebrew);
    DMR_NULL_CHECK_FREE(homebrew->txq = dmr_packetq_new(), homebrew);

    /* Setup buffer */
    buffer_t *buffer = buffer_new(302);
    DMR_NULL_CHECK_FREE(buffer, homebrew);
    homebrew->buffer = buffer;

    return homebrew;
}

DMR_API int dmr_homebrew_auth(dmr_homebrew *homebrew, char *secret)
{
    if (homebrew == NULL || secret == NULL || strlen(secret) == 0)
        return dmr_error(DMR_EINVAL);

    if (homebrew->config.repeater_id == 0) {
        dmr_log_error("homebrew: repeater_id can't be 0");
        return dmr_error(DMR_EINVAL);
    }

    dmr_log_info("homebrew: connecting to [%s]:%u as %u (%s)",
        format_ip6s(homebrew->peer_ip), homebrew->peer_port,
        homebrew->config.repeater_id,
        homebrew->config.call);

    homebrew->state = DMR_HOMEBREW_AUTH_NONE;
    buffer_t *buffer = (buffer_t *)homebrew->buffer;
    buffer_reset(buffer);
    buffer_cat(buffer, "RPTL", 4);
    buffer_xuint32(buffer, homebrew->config.repeater_id);
    dmr_homebrew_send_raw(homebrew, buffer->buf, buffer->len);
}
