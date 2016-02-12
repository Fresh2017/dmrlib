#include <string.h>
#include <netinet/ip.h>
#include "dmr/error.h"
#include "dmr/malloc.h"
#include "dmr/id.h"
#include "dmr/raw.h"
#include "dmr/protocol/homebrew.h"
#include "dmr/version.h"
#include "common/byte.h"
#include "common/buffer.h"
#include "common/format.h"
#include "common/scan.h"
#include "common/sha256.h"
#include "common/socket.h"

DMR_PRV static const char *homebrew_location = "Earth";
DMR_PRV static const char *homebrew_description = "dmrlib";
DMR_PRV static const char *homebrew_software_id = DMRLIB_SOFTWARE_ID;
DMR_PRV static const char *homebrew_package_id = DMRLIB_PACKAGE_ID;
DMR_PRV static const char *homebrew_url = "https://github.com/pd0mz/dmrlib";

DMR_PRV static int homebrew_send_config(dmr_homebrew *homebrew);
DMR_PRV static int homebrew_send_key(dmr_homebrew *homebrew);

DMR_API dmr_homebrew *dmr_homebrew_new(dmr_id repeater_id, uint8_t peer_ip[16], uint16_t peer_port, uint8_t bind_ip[16], uint16_t bind_port)
{
    /* Setup homebrew struct */
    DMR_MALLOC_CHECK(dmr_homebrew, homebrew);

    /* Setup ident */
    DMR_NULL_CHECK_FREE(homebrew->id = dmr_palloc_size(homebrew, 24), homebrew);
    snprintf(homebrew->id, 24, "homebrew[%u]", repeater_id);
    homebrew->config.repeater_id = repeater_id;

    /* Setup addresses */
    if (peer_port == 0)
        homebrew->peer_port = DMR_HOMEBREW_PORT;
    else
        homebrew->peer_port = peer_port;

    if (byte_equal(peer_ip, ip6any, 16)) {
        DMR_HB_FATAL("invalid peer address");
        dmr_free(homebrew);
        return NULL;
    } 
    byte_copy(homebrew->peer_ip, peer_ip, 16);
    byte_copy(homebrew->bind_ip, bind_ip, 16);
    homebrew->bind_port = bind_port;

    char bind_str[FORMAT_IP6_LEN], peer_str[FORMAT_IP6_LEN];
    byte_zero(bind_str, sizeof bind_str);
    byte_zero(peer_str, sizeof peer_str);
    format_ip6(bind_str, homebrew->bind_ip);
    format_ip6(peer_str, homebrew->peer_ip);
    DMR_HB_DEBUG("new [%s]:%u to [%s]:%u",
        bind_str, bind_port, peer_str, peer_port);

    /* Setup socket */
    socket_t *sock = socket_udp6(0);
    DMR_NULL_CHECK_FREE(sock, homebrew);
    if (!byte_equal(homebrew->bind_ip, ip6any, 16) || homebrew->bind_port) {
        if (socket_bind(sock, homebrew->bind_ip, homebrew->bind_port) == -1) {
            DMR_HB_FATAL("bind to [%s]:%u failed: %s",
                format_ip6s(homebrew->bind_ip), homebrew->bind_port,
                strerror(errno));
            socket_close(sock);
            dmr_free(homebrew);
            return NULL;
        }       
    }
    homebrew->sock = sock;

    return homebrew;
}

DMR_API int dmr_homebrew_auth(dmr_homebrew *homebrew, char *secret)
{
    DMR_ERROR_IF_NULL(homebrew, DMR_EINVAL);
    if (secret == NULL) {
        DMR_HB_ERROR("can't authenticate with NULL secret");
        return dmr_error(DMR_EINVAL);
    }

    if (homebrew->config.repeater_id == 0) {
        DMR_HB_ERROR("repeater_id can't be 0");
        return dmr_error(DMR_EINVAL);
    }
    if (homebrew->secret != NULL)
        TALLOC_FREE(homebrew->secret);

    DMR_ERROR_IF_NULL(homebrew->secret = dmr_strdup(homebrew, secret), DMR_ENOMEM);

    const char *call = dmr_id_name(homebrew->config.repeater_id);
    if (call == NULL)
        call = "?";
    DMR_HB_INFO("connecting to [%s]:%u as %u (%s) with call %s",
        format_ip6s(homebrew->peer_ip), homebrew->peer_port,
        homebrew->config.repeater_id, call,
        homebrew->config.call);
    homebrew->state = DMR_HOMEBREW_AUTH_NONE;
    dmr_raw *raw = dmr_raw_new(12); /* malloc, freed in writer/closer */
    DMR_ERROR_IF_NULL(raw, DMR_ENOMEM);
    dmr_raw_add(raw, "RPTL", 4);
    dmr_raw_add_xuint32(raw, homebrew->config.repeater_id);
    return dmr_homebrew_send_raw(homebrew, raw);
}

DMR_API int dmr_homebrew_close(dmr_homebrew *homebrew)
{
    DMR_ERROR_IF_NULL(homebrew, DMR_EINVAL);

    if (homebrew->state == DMR_HOMEBREW_AUTH_NONE)
        return 0;

    dmr_raw *raw = dmr_raw_new(12); /* malloc, freed in writer */
    DMR_ERROR_IF_NULL(raw, DMR_ENOMEM);
    dmr_raw_add(raw, "RPTC", 4);
    dmr_raw_add_xuint32(raw, homebrew->config.repeater_id);
    return dmr_homebrew_send_raw(homebrew, raw);
}

DMR_API int dmr_homebrew_send_buf(dmr_homebrew *homebrew, uint8_t *buf, size_t len)
{
    DMR_ERROR_IF_NULL(homebrew, DMR_EINVAL);
    DMR_ERROR_IF_NULL(buf, DMR_EINVAL);

    if (len == 0)
        return 0;

    dmr_raw *raw = dmr_raw_new(len); /* malloc, freed by send_raw */
    DMR_ERROR_IF_NULL(raw, DMR_ENOMEM);
    dmr_raw_add(raw, buf, len);
    return dmr_homebrew_send_raw(homebrew, raw);
}

DMR_API int dmr_homebrew_send_raw(dmr_homebrew *homebrew, dmr_raw *raw)
{
    DMR_ERROR_IF_NULL(homebrew, DMR_EINVAL);
    if (raw == NULL) {
        DMR_HB_ERROR("can't send NULL raw");
        return dmr_error(DMR_EINVAL);
    }

#if defined(DMR_TRACE)
    DMR_HB_DEBUG("send([%s]:%u): %lu bytes",
        format_ip6s(homebrew->peer_ip), homebrew->peer_port, raw->len);
    dmr_dump_hex(raw->buf, raw->len);
#endif

    socket_t *sock = (socket_t *)homebrew->sock;
    if (sock == NULL) {
        DMR_HB_ERROR("can't send, protocol not setup");
        return dmr_error(DMR_EINVAL);
    }

    int ret = 0;
    do {
        ret = socket_send(sock, raw->buf, raw->len, homebrew->peer_ip, homebrew->peer_port);
    } while (ret == -1 && (errno == EINVAL || errno == EAGAIN));
    if (ret == -1) {
        DMR_HB_ERROR("send([%s]:%u,%llu): %s",
            format_ip6s(homebrew->peer_ip), homebrew->peer_port,
            raw->len, strerror(errno));
    } else {
        ret = 0;
    }

    dmr_raw_free(raw);
    return ret;
}

DMR_API int dmr_homebrew_read(dmr_homebrew *homebrew, dmr_parsed_packet **parsed_out)
{
    socket_t *sock = (socket_t *)homebrew->sock;
    /* max packet size by a repeater is a DMRD frame of 53 bytes */
    dmr_raw *raw = dmr_raw_new(53); 
    DMR_ERROR_IF_NULL(raw, DMR_ENOMEM);
    ip6_t peer_ip;
    uint16_t peer_port;
    ssize_t len = socket_recv(sock, raw->buf, raw->allocated, peer_ip, &peer_port);
    if (len < 0) {
        DMR_HB_ERROR("recv: %s", strerror(errno));
        return -1;
    } else if (len < 14) {
        /* shortest packet the repeater can send is a 14 byte MSTACK/MSTNAK */
        DMR_HB_WARN("repeater sent short packet");
        return 0;
    } else {
        raw->len = (uint64_t)len;
    }

#if defined(DMR_TRACE)
    DMR_HB_DEBUG("recv([%s]:%u): %lu bytes",
        format_ip6s(homebrew->peer_ip), homebrew->peer_port, raw->len);
    dmr_dump_hex(raw->buf, raw->len);
#endif

    int ret = 0;
    switch (raw->buf[0]) {
    case 'D': /* DMRD */
        if (len == 53) {
            DMR_HB_DEBUG("repeater sent DMR data");
            if (parsed_out != NULL) {
                ret = dmr_homebrew_parse_dmrd(homebrew, raw, parsed_out);
            }
        }
        break;

    case 'M': /* MSTNAK, MSTACK, MSTCL */
        if (len == 14) { /* MSTNAK, MSTACK (without nonce) */
            if (!byte_cmp(raw->buf + 3, "ACK", 3)) {
                DMR_HB_DEBUG("repeater ack");
                switch (homebrew->state) {
                case DMR_HOMEBREW_AUTH_NONE:
                    DMR_HB_DEBUG("repeater didn't respond with a nonce");
                    break;
                case DMR_HOMEBREW_AUTH_INIT:
                    DMR_HB_DEBUG("repeater accepted our key, sending config");
                    ret = homebrew_send_config(homebrew);
                    break;
                case DMR_HOMEBREW_AUTH_CONFIG:
                    DMR_HB_INFO("repeater login successful");
                    gettimeofday(&homebrew->last_ping, NULL);
                    gettimeofday(&homebrew->last_pong, NULL);
                    homebrew->state = DMR_HOMEBREW_AUTH_DONE;
                    break;
                default:
                    DMR_HB_DEBUG("repeater ACK (ignored)");
                    break;
                }
                break;
            } else if (!byte_cmp(raw->buf + 3, "NAK", 3)) {
                DMR_HB_DEBUG("repeater NAK");
                DMR_HB_ERROR("repeater NAK, auth again");
                homebrew->state = DMR_HOMEBREW_AUTH_NONE;
                /* request closing from the I/O loop */
                ret = -1;
            }

        } else if (len == 22) { /* MSTACK (with nonce) */
            if (!byte_cmp(raw->buf + 3, "ACK", 3)) {
                DMR_HB_DEBUG("repeater ACK with nonce");
                if (homebrew->state == DMR_HOMEBREW_AUTH_NONE) {
                    byte_copy(homebrew->nonce, raw->buf + 14, 8);
                    ret = homebrew_send_key(homebrew);
                }
            }
        } else {
            DMR_HB_WARN("master sent unknown response \"%.*s\"",
                 raw->len, raw->buf);
        }
        break;

    case 'R': /* RPTSBKN, RPTPONG */
        if (len == 15 && !byte_cmp(raw->buf + 3, "PONG", 4)) {
            gettimeofday(&homebrew->last_pong, NULL);
            struct timeval latency;
            timersub(&homebrew->last_pong, &homebrew->last_ping, &latency);
            uint32_t ms = ((latency.tv_sec * 1000) + (latency.tv_usec / 1000));
            DMR_HB_DEBUG("repeater pong (%ums latency)", ms);
        } else if (len == 15 && !byte_cmp(raw->buf + 3, "SBKN", 4)) {
            DMR_HB_DEBUG("repeater site beacon (ignored)");
        } else {
            DMR_HB_WARN("master sent unknown response \"%.*s\"",
                 raw->len, raw->buf);
        }
        break;

    default:
        DMR_HB_WARN("master sent unknown response \"%.*s\"",
             raw->len, raw->buf);
    }

    dmr_raw_free(raw);
    return ret;
}

DMR_API int dmr_homebrew_send(dmr_homebrew *homebrew, dmr_parsed_packet *parsed)
{
    DMR_ERROR_IF_NULL(homebrew, DMR_EINVAL);
    DMR_ERROR_IF_NULL(parsed, DMR_EINVAL);

    /* construct 53 byte DMRD frame */
    dmr_raw *raw = dmr_raw_new(53); /* malloc, freed by send_raw */
    DMR_ERROR_IF_NULL(raw, DMR_ENOMEM);

    uint8_t slot_info = 0;
    /* handle DMO mode */
    if (homebrew->config.rx_freq != homebrew->config.tx_freq)
        slot_info |= parsed->ts;
    if (parsed->flco == DMR_FLCO_PRIVATE)
        slot_info |= 0x02;
    if (parsed->data_type == DMR_DATA_TYPE_INVALID ||
        parsed->data_type == DMR_DATA_TYPE_VOICE) {
        slot_info |= 0x00;
        slot_info |= (parsed->voice_frame) << 4;
    }
    else if (parsed->data_type == DMR_DATA_TYPE_VOICE_SYNC)
        slot_info |= 0x04;
    else {
        slot_info |= 0x08;
        slot_info |= (parsed->data_type) << 4;
    }

    dmr_raw_add(raw, "DMRD", 4);
    dmr_raw_add_uint8(raw, parsed->sequence & 0xff);
    dmr_raw_add_uint24(raw, parsed->src_id);
    dmr_raw_add_uint24(raw, parsed->dst_id);
    dmr_raw_add_uint32(raw, parsed->repeater_id);
    dmr_raw_add_uint8(raw, slot_info);
    dmr_raw_add_uint32(raw, parsed->stream_id);
    dmr_raw_add(raw, parsed->packet, DMR_PACKET_LEN);
    
    return dmr_homebrew_send_raw(homebrew, raw);
}

DMR_API int dmr_homebrew_parse_dmrd(dmr_homebrew *homebrew, dmr_raw *raw, dmr_parsed_packet **parsed_out)
{
    DMR_ERROR_IF_NULL(homebrew, DMR_EINVAL);
    DMR_ERROR_IF_NULL(raw, DMR_EINVAL);

    if (raw->len != 53) {
        DMR_HB_ERROR("not a DMRD frame");
        *parsed_out = NULL;
        return dmr_error(DMR_EINVAL);
    }

    dmr_parsed_packet *parsed;
    DMR_ERROR_IF_NULL(parsed = dmr_malloc(dmr_parsed_packet), DMR_ENOMEM);

    /* parse DMRD frame */
    parsed->sequence = raw->buf[4];
    parsed->src_id = uint24(raw->buf + 5);
    parsed->dst_id = uint24(raw->buf + 8);
    parsed->repeater_id = uint32(raw->buf + 11); 
    parsed->ts = (dmr_ts)(raw->buf[15] & 0x01);
    parsed->flco = (dmr_flco)((raw->buf[15] & 0x02) >> 1);
    switch ((raw->buf[15] >> 2) & 0x03) {
    case 0x00:
        parsed->data_type = DMR_DATA_TYPE_VOICE;
        parsed->voice_frame = (raw->buf[15] >> 4);
        break;
    case 0x01:
        parsed->data_type = DMR_DATA_TYPE_VOICE_SYNC;
        break;
    case 0x02:
        parsed->data_type = (raw->buf[15] >> 4);
        break;
    }
    parsed->stream_id = uint32(raw->buf + 16);
    byte_copy(parsed->packet, raw->buf + 20, DMR_PACKET_LEN);

    const char *src_call = dmr_id_name(parsed->src_id);
    const char *dst_call = dmr_id_name(parsed->dst_id);
    DMR_HB_DEBUG("%s/%02x, type=%s from %u(%s)->%u(%s), privacy=%u, stream=%08x",
        dmr_ts_name(parsed->ts), parsed->sequence,
        dmr_data_type_name(parsed->data_type),
        parsed->src_id, src_call == NULL ? "?" : src_call,
        parsed->dst_id, dst_call == NULL ? "?" : dst_call,
        parsed->flco, parsed->stream_id);
    *parsed_out = parsed;

    return 0;
}

/* Private functions */

DMR_PRV static int homebrew_send_config(dmr_homebrew *homebrew)
{
    dmr_raw *raw;
    DMR_ERROR_IF_NULL(raw = dmr_raw_new(302), DMR_ENOMEM);

    DMR_HB_DEBUG("sending config");

    dmr_raw_add(raw, "RPTC", 4);
    dmr_raw_addf(raw, 8, "%-8s", homebrew->config.call);
    dmr_raw_addf(raw, 8, "%08x", homebrew->config.repeater_id);
    dmr_raw_addf(raw, 9, "%09u", homebrew->config.rx_freq);
    dmr_raw_addf(raw, 9, "%09u", homebrew->config.tx_freq);
    dmr_raw_addf(raw, 2, "%02d", MIN(homebrew->config.tx_power, 99));
    dmr_raw_addf(raw, 2, "%02d", homebrew->config.color_code);
    dmr_raw_addf(raw, 8, "%08.04f", homebrew->config.latitude);
    dmr_raw_addf(raw, 9, "%09.04f", homebrew->config.longitude);
    dmr_raw_addf(raw, 3, "%03d", MIN(homebrew->config.altitude, 999));
#define D(size,fmt,field) do { \
    if (homebrew->config.field == NULL) \
        dmr_raw_addf(raw, size, fmt, homebrew_ ##field); \
    else \
        dmr_raw_addf(raw, size, fmt, homebrew->config.field); \
} while (0)
    D(20,  "%-20s",  location);
    D(20,  "%-20s",  description);
    D(124, "%-124s", url);
    D(40,  "%-40s",  software_id);
    D(40,  "%-40s",  package_id);
#undef D

    homebrew->state = DMR_HOMEBREW_AUTH_CONFIG;
    return dmr_homebrew_send_raw(homebrew, raw);
}

DMR_PRV static int homebrew_send_key(dmr_homebrew *homebrew)
{
    uint8_t digest[SHA256_DIGEST_LENGTH];
    sha256_t sha256ctx;
    dmr_raw *raw;
    DMR_ERROR_IF_NULL(raw = dmr_raw_new(76), DMR_ENOMEM);

    DMR_HB_DEBUG("sending key");

    sha256_init(&sha256ctx);
    sha256_update(&sha256ctx, homebrew->nonce, 8);
    sha256_update(&sha256ctx, (const uint8_t *)homebrew->secret, strlen(homebrew->secret));
    sha256_final(&sha256ctx, digest);

    dmr_raw_add(raw, "RPTK", 4);
    dmr_raw_add_xuint32(raw, homebrew->config.repeater_id);
    dmr_raw_add_hex(raw, digest, SHA256_DIGEST_LENGTH);
    raw->len += SHA256_DIGEST_LENGTH;

    homebrew->state = DMR_HOMEBREW_AUTH_INIT;
    return dmr_homebrew_send_raw(homebrew, raw);
}
