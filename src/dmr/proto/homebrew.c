#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <talloc.h>
#ifdef DMR_DEBUG
#include <assert.h>
#endif
#include "dmr/bits.h"
#include "dmr/error.h"
#include "dmr/log.h"
#include "dmr/proto.h"
#include "dmr/proto/homebrew.h"
#include "dmr/type.h"
#include "sha256.h"

static const char *dmr_homebrew_proto_name = "homebrew";
static const char hex[16] = "0123456789abcdef";

static const char dmr_homebrew_data_signature[4]   = "DMRD";
static const char dmr_homebrew_master_ack[6]       = "MSTACK";
static const char dmr_homebrew_master_nak[6]       = "MSTNAK";
static const char dmr_homebrew_master_ping[7]      = "MSTPING";
static const char dmr_homebrew_master_closing[5]   = "MSTCL";
static const char dmr_homebrew_repeater_login[4]   = "RPTL";
static const char dmr_homebrew_repeater_key[4]     = "RPTK";
static const char dmr_homebrew_repeater_pong[7]    = "RPTPONG";
static const char dmr_homebrew_repeater_closing[5] = "RPTCL";
static const char dmr_homebrew_repeater_beacon[7]  = "RPTSBKN";
static const char dmr_homebrew_repeater_rssi[7]    = "RPTRSSI";

static int homebrew_proto_init(void *homebrewptr)
{
    dmr_log_debug("homebrew: init");
    dmr_homebrew_t *homebrew = (dmr_homebrew_t *)homebrewptr;
    if (homebrew == NULL)
        return dmr_error(DMR_EINVAL);
    else if (homebrew->auth != DMR_HOMEBREW_AUTH_DONE) {
        dmr_log_error("homebrew: authentication not done, did you call homebrew_auth?");
        return dmr_error(DMR_EINVAL);
    }

    srand((unsigned int)time(NULL));

    homebrew->proto.init_done = true;
    return 0;
}

static int homebrew_proto_start_thread(void *homebrewptr)
{
    dmr_log_debug("homebrew: start thread %d", dmr_thread_id(NULL) % 1000);
    dmr_homebrew_t *homebrew = (dmr_homebrew_t *)homebrewptr;
    if (homebrew == NULL) {
        return dmr_thread_error;
    }

    dmr_thread_name_set("homebrew proto");
    dmr_homebrew_loop(homebrew);
    dmr_thread_exit(dmr_thread_success);
    return dmr_thread_success;
}

static int homebrew_proto_start(void *homebrewptr)
{
    dmr_log_debug("homebrew: start");
    dmr_homebrew_t *homebrew = (dmr_homebrew_t *)homebrewptr;
    if (homebrew == NULL)
        return dmr_error(DMR_EINVAL);

    if (!homebrew->proto.init_done) {
        dmr_log_error("homebrew: attempt to start without init");
        return dmr_error(DMR_EINVAL);
    }

    if (homebrew->proto.thread != NULL) {
        dmr_log_error("homebrew: can't start, already active");
        return dmr_error(DMR_EINVAL);
    }

    homebrew->proto.thread = malloc(sizeof(dmr_thread_t));
    if (homebrew->proto.thread == NULL) {
        return dmr_error(DMR_ENOMEM);
    }

    if (dmr_thread_create(homebrew->proto.thread, homebrew_proto_start_thread, homebrewptr) != dmr_thread_success) {
        dmr_log_error("homebrew: can't create thread");
        return dmr_error(DMR_EINVAL);
    }

    return 0;
}

static int homebrew_proto_stop(void *homebrewptr)
{
    dmr_log_debug("homebrew: stop");
    dmr_homebrew_t *homebrew = (dmr_homebrew_t *)homebrewptr;
    if (homebrew == NULL)
        return dmr_error(DMR_EINVAL);

    if (homebrew->proto.thread == NULL) {
        dmr_log_info("homebrew: not active");
        return 0;
    }

    dmr_mutex_lock(homebrew->proto.mutex);
    homebrew->proto.is_active = false;
    dmr_mutex_unlock(homebrew->proto.mutex);
    if (dmr_thread_join(*homebrew->proto.thread, NULL) != dmr_thread_success) {
        dmr_log_error("homebrew: can't join thread");
        return dmr_error(DMR_EINVAL);
    }

    free(homebrew->proto.thread);
    homebrew->proto.thread = NULL;
    return 0;
}

static bool homebrew_proto_active(void *homebrewptr)
{
    dmr_log_trace("homebrew: active");
    dmr_homebrew_t *homebrew = (dmr_homebrew_t *)homebrewptr;
    if (homebrew == NULL)
        return false;

    dmr_mutex_lock(homebrew->proto.mutex);
    bool active = homebrew->proto.thread != NULL && homebrew->proto.is_active;
    dmr_mutex_unlock(homebrew->proto.mutex);
    return active;
}

static int homebrew_proto_wait(void *homebrewptr)
{
    dmr_log_debug("homebrew: wait");
    dmr_homebrew_t *homebrew = (dmr_homebrew_t *)homebrewptr;
    if (homebrew == NULL)
        return 0;
    if (!homebrew_proto_active(homebrew))
        return 0;
    return dmr_thread_join(*homebrew->proto.thread, NULL);
}

static void homebrew_proto_rx(void *homebrewptr, dmr_packet_t *packet)
{
    dmr_log_trace("homebrew: proto rx");
    dmr_homebrew_t *homebrew = (dmr_homebrew_t *)homebrewptr;
    if (homebrew == NULL || packet == NULL)
        return;

    dmr_log_debug("homebrew: received %s packet",
        dmr_data_type_name(packet->data_type));

    dmr_proto_rx_cb_run(&homebrew->proto, packet);
}

static void homebrew_proto_tx(void *homebrewptr, dmr_packet_t *packet)
{
    dmr_log_debug("homebrew: proto tx");
    dmr_homebrew_t *homebrew = (dmr_homebrew_t *)homebrewptr;
    if (homebrew == NULL || packet == NULL)
        return;

    packet->repeater_id = homebrew->id;
    dmr_homebrew_send(homebrew, packet->ts, packet);
}

dmr_homebrew_t *dmr_homebrew_new(struct in_addr bind, int port, struct in_addr peer)
{
    dmr_log_debug("homebrew: new on %s:%d to %s:%d",
        inet_ntoa(bind), port,
        inet_ntoa(peer), port);

    int optval = 1;
    dmr_homebrew_t *homebrew = talloc_zero(NULL, dmr_homebrew_t);
    if (homebrew == NULL) {
        DMR_OOM();
        return NULL;
    }

    dmr_homebrew_config_init(&homebrew->config);

    uint8_t i;
    for (i = 0; i < 2; i++) {
        memset(&homebrew->tx[i].last_voice_packet_sent, 0, sizeof(struct timeval));
        memset(&homebrew->tx[i].last_data_packet_sent, 0, sizeof(struct timeval));
    }

    // Setup protocol
    homebrew->proto.name = dmr_homebrew_proto_name;
    homebrew->proto.type = DMR_PROTO_HOMEBREW;
    homebrew->proto.init = homebrew_proto_init;
    homebrew->proto.start = homebrew_proto_start;
    homebrew->proto.stop = homebrew_proto_stop;
    homebrew->proto.wait = homebrew_proto_wait;
    homebrew->proto.active = homebrew_proto_active;
    homebrew->proto.rx = homebrew_proto_rx;
    homebrew->proto.tx = homebrew_proto_tx;
    if (dmr_proto_mutex_init(&homebrew->proto) != 0) {
        dmr_log_error("homebrew: failed to init mutex");
        talloc_free(homebrew);
        return NULL;
    }

    // Setup file descriptor for UDP socket
    homebrew->fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (homebrew->fd < 0) {
        dmr_error_set(strerror(errno));
        talloc_free(homebrew);
        return NULL;
    }

    setsockopt(homebrew->fd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval, sizeof(int));

    homebrew->server.sin_family = AF_INET;
    homebrew->server.sin_addr.s_addr = bind.s_addr;
    homebrew->server.sin_port = htons(port);
    memset(homebrew->server.sin_zero, 0, sizeof(homebrew->server.sin_zero));

    homebrew->remote.sin_family = AF_INET;
    homebrew->remote.sin_addr.s_addr = peer.s_addr;
    homebrew->remote.sin_port = htons(port);
    memset(homebrew->remote.sin_zero, 0, sizeof(homebrew->remote.sin_zero));

    return homebrew;
}

int dmr_homebrew_auth(dmr_homebrew_t *homebrew, const char *secret)
{
    uint8_t buf[328], digest[SHA256_DIGEST_LENGTH];
    int i, j, ret;
    ssize_t len;
    sha256_t sha256ctx;
    //memset(&buf, 0, 64);

    dmr_log_info("homebrew: connecting to repeater at %s:%d as %s",
        inet_ntoa(homebrew->remote.sin_addr),
        ntohs(homebrew->remote.sin_port),
        homebrew->config.repeater_id);

    while (homebrew->auth != DMR_HOMEBREW_AUTH_DONE) {
        switch (homebrew->auth) {
        case DMR_HOMEBREW_AUTH_NONE:
            memcpy(buf, dmr_homebrew_repeater_login, 4);
            memcpy(buf + 4, homebrew->config.repeater_id, 8);
            dmr_log_trace("homebrew: sending repeater id");
            if ((ret = dmr_homebrew_sendraw(homebrew, buf, 12)) < 0)
                return ret;

            while (true) {
                if ((ret = dmr_homebrew_recvraw(homebrew, &len, NULL)) < 0)
                    return ret;

                if (len == 14 && !memcmp(&homebrew->buffer[0], dmr_homebrew_master_nak, 6)) {
                    homebrew->auth = DMR_HOMEBREW_AUTH_FAIL;
                    return dmr_error_set("homebrew: master refused our DMR ID");
                }
                if (len == 22 && !memcmp(&homebrew->buffer[0], dmr_homebrew_master_ack, 6)) {
                    memcpy(&homebrew->random, &homebrew->buffer[14], 8);
                    dmr_log_trace("homebrew: master sent nonce %.*s",
                        8, homebrew->random);
                    dmr_log_debug("homebrew: master accepted our repeater id");
                    homebrew->auth = DMR_HOMEBREW_AUTH_INIT;
                    break;
                }

                // We could be receiving DMRD frames, or other data here, which
                // we'll ignore at this stage.
            }
            break;

        case DMR_HOMEBREW_AUTH_INIT:
            memcpy(buf, homebrew->random, 8);
            memcpy(buf + 8, secret, strlen(secret));
            sha256_init(&sha256ctx);
            sha256_update(&sha256ctx, (const uint8_t *)homebrew->random, 8);
            sha256_update(&sha256ctx, (const uint8_t *)secret, strlen(secret));
            sha256_final(&sha256ctx, &digest[0]);

            memcpy(buf, dmr_homebrew_repeater_key, 4);
            memcpy(buf + 4, homebrew->config.repeater_id, 8);
            for (i = 0, j = 0; i < SHA256_DIGEST_LENGTH; i++, j += 2) {
                buf[12 + j] = hex[(digest[i] >> 4)];
                buf[13 + j] = hex[(digest[i] & 0x0f)];
            }

            dmr_log_trace("homebrew: sending nonce");
            if ((ret = dmr_homebrew_sendraw(homebrew, buf, 12 + 64)) < 0)
                return ret;

            while (true) {
                if ((ret = dmr_homebrew_recvraw(homebrew, &len, NULL)) < 0)
                    return ret;

                if (len == 14 && !memcmp(&homebrew->buffer[0], dmr_homebrew_master_nak, 6)) {
                    homebrew->auth = DMR_HOMEBREW_AUTH_FAIL;
                    return dmr_error_set("homebrew: master authentication failed");
                }
                if (len == 14 && !memcmp(&homebrew->buffer[0], dmr_homebrew_master_ack, 6)) {
                    dmr_log_debug("homebrew: master accepted nonce, logged in");
                    homebrew->auth = DMR_HOMEBREW_AUTH_CONF;
                    break;
                }

            }
            break;

        case DMR_HOMEBREW_AUTH_FAIL:
            return dmr_error_set("homebrew: master authentication failed");

        case DMR_HOMEBREW_AUTH_CONF:
            dmr_log_trace("homebrew: logged in, sending our configuration");
#ifdef DMR_DEBUG
            assert(sizeof(dmr_homebrew_config_t) == 306);
#endif
            memcpy(buf, &homebrew->config, sizeof(dmr_homebrew_config_t));
            if ((ret = dmr_homebrew_sendraw(homebrew, buf, sizeof(dmr_homebrew_config_t))) < 0)
                return ret;

            homebrew->auth = DMR_HOMEBREW_AUTH_DONE;
            gettimeofday(&homebrew->last_ping_sent, NULL);
            break;

        case DMR_HOMEBREW_AUTH_DONE:
            break;
        }
    }

    return 0;
}

void dmr_homebrew_close(dmr_homebrew_t *homebrew)
{
    uint8_t buf[12];

    if (homebrew == NULL)
        return;

    if (homebrew->fd < 0)
        return;

    dmr_mutex_lock(homebrew->proto.mutex);
    if (homebrew->proto.is_active) {
        homebrew->proto.is_active = false;
    dmr_mutex_unlock(homebrew->proto.mutex);
#ifdef DMR_PLATFORM_WINDOWS
        Sleep(5000);
#else
        sleep(5);
#endif
    }

    memcpy(buf, dmr_homebrew_repeater_closing, 4);
    memcpy(buf + 4, homebrew->config.repeater_id, 8);
    dmr_homebrew_sendraw(homebrew, buf, 12);
}

void dmr_homebrew_free(dmr_homebrew_t *homebrew)
{
    if (homebrew == NULL)
        return;

    dmr_mutex_lock(homebrew->proto.mutex);
    if (homebrew->proto.is_active) {
        dmr_mutex_unlock(homebrew->proto.mutex);
        dmr_homebrew_close(homebrew); // also may lock the mutex
#ifdef DMR_PLATFORM_WINDOWS
        Sleep(1000);
#else
        sleep(1);
#endif

        close(homebrew->fd);
    } else {
        dmr_mutex_unlock(homebrew->proto.mutex);
    }

    talloc_free(homebrew);
}

static void dmr_homebrew_dump_data(dmr_packet_t *packet)
{
    if (packet == NULL)
        return;

    dmr_log_debug("homebrew: frame [%lu->%lu] via %lu, seq 0x%02x, slot %d, call type %d, data type %s (%d), stream id 0x%08lx:",
        packet->src_id,
        packet->dst_id,
        packet->repeater_id,
        packet->meta.sequence,
        packet->ts,
        packet->flco,
        dmr_data_type_name_short(packet->data_type),
        packet->data_type,
        packet->meta.stream_id);

    dump_hex(packet->payload, DMR_PAYLOAD_BYTES);
}

static uint32_t dmr_homebrew_generate_stream_id(void)
{
    // Not going to check how much RAND_MAX is, just grab 4 bytes and pack them
    return DMR_UINT32_BE(rand(), rand(), rand(), rand());
}

void dmr_homebrew_loop(dmr_homebrew_t *homebrew)
{
    uint8_t buf[53];
    ssize_t len = 0;
    int ret;

    if (homebrew == NULL)
        return;

    dmr_log_debug("homebrew: loop");
    dmr_mutex_lock(homebrew->proto.mutex);
    homebrew->proto.is_active = true;
    dmr_mutex_unlock(homebrew->proto.mutex);
    while (homebrew_proto_active(homebrew)) {
        struct timeval timeout = { 1, 0 };

        if (dmr_time_since(homebrew->last_ping_sent) > 3) {
            dmr_log_debug("homebrew: pinging master");
            memset(buf, 0, sizeof(buf));
            memcpy(buf, dmr_homebrew_master_ping, 7);
            memcpy(buf + 7, homebrew->config.repeater_id, 8);
            if ((ret = dmr_homebrew_sendraw(homebrew, buf, 15)) != 0) {
                dmr_log_error("homebrew: error sending ping: %s", dmr_error_get());
                break;
            }
            gettimeofday(&homebrew->last_ping_sent, NULL);
        }

        if ((ret = dmr_homebrew_recvraw(homebrew, &len, &timeout)) != 0) {
            // Only log errors if it's not a timeout
            if (ret == -1)
                continue;

            dmr_log_error("homebrew: loop error: %s", dmr_error_get());
            break;
        }

        if (len == 15 && !memcmp(homebrew->buffer, dmr_homebrew_master_ping, 7)) {
            dmr_log_debug("homebrew: ping? pong!");
            memcpy(buf, homebrew->buffer, 15);
            memcpy(buf, dmr_homebrew_repeater_pong, 7);
            if (!dmr_homebrew_sendraw(homebrew, buf, 15))
                return;
        } else if (len == 53 && !memcmp(homebrew->buffer, dmr_homebrew_data_signature, 4)) {
            dmr_log_debug("homebrew: got data packet");
            dmr_packet_t *packet = dmr_homebrew_parse_packet(homebrew->buffer, len);
            if (packet == NULL) {
                dmr_error(DMR_ENOMEM);
                return;
            }
            if (dmr_log_priority() <= DMR_LOG_PRIORITY_DEBUG) {
                dump_hex(packet->payload, DMR_PAYLOAD_BYTES);
            }
            dmr_homebrew_dump_data(packet);
            homebrew->proto.rx(homebrew, packet);
            free(packet);
        } else if (len > 7 && !memcmp(homebrew->buffer, dmr_homebrew_repeater_pong, 7)) {
            dmr_log_debug("homebrew: master sent pong");
        } else if (len > 7 && !memcmp(homebrew->buffer, dmr_homebrew_repeater_beacon, 7)) {
            dmr_log_debug("homebrew: master sent synchronous site beacon (ignored)");
        } else if (len > 7 && !memcmp(homebrew->buffer, dmr_homebrew_repeater_rssi, 7)) {
            dmr_log_debug("homebrew: master sent repeater RSSI (ignored)");
        } else if (len == 12 && !memcmp(homebrew->buffer, dmr_homebrew_master_ack, 6)) {
            dmr_log_debug("homebrew: master sent ack");
        } else if (len == 12 && !memcmp(homebrew->buffer, dmr_homebrew_master_closing, 5)) {
            dmr_log_critical("homebrew: master closing");
            break;
        } else {
            dmr_log_debug("homebrew: master sent unknown data");
        }
    }

    dmr_log_debug("homebrew: loop finished");
}

int dmr_homebrew_send(dmr_homebrew_t *homebrew, dmr_ts_t ts, dmr_packet_t *packet)
{
    if (homebrew == NULL || packet == NULL || ts > DMR_TS2)
        return dmr_error(DMR_EINVAL);

    if (packet->repeater_id == 0)
        packet->repeater_id = homebrew->id;

    uint8_t buf[DMR_PAYLOAD_BYTES + 20];
    buf[0x00] = 'D';
    buf[0x01] = 'M';
    buf[0x02] = 'R';
    buf[0x03] = 'D';
    buf[0x04] = packet->meta.sequence;
    buf[0x05] = packet->src_id >> 16;
    buf[0x06] = packet->src_id >> 8;
    buf[0x07] = packet->src_id >> 0;
    buf[0x08] = packet->src_id >> 16;
    buf[0x09] = packet->src_id >> 8;
    buf[0x0a] = packet->src_id >> 0;
    buf[0x0b] = packet->repeater_id >> 24;
    buf[0x0c] = packet->repeater_id >> 16;
    buf[0x0d] = packet->repeater_id >> 8;
    buf[0x0e] = packet->repeater_id >> 0;
    buf[0x0f]  = 0;
    buf[0x0f] |= (packet->ts   & B00000001) << 7;
    buf[0x0f] |= (packet->flco & B00000001) << 6;
    switch (packet->data_type) {
    case DMR_DATA_TYPE_VOICE_SYNC:
        buf[0x0f] |= B00010000;
        break;
    case DMR_DATA_TYPE_VOICE:
        buf[0x0f] |= packet->meta.voice_frame;
        break;
    default:
        if ((packet->data_type == DMR_DATA_TYPE_VOICE_LC || packet->data_type == DMR_DATA_TYPE_DATA) && packet->meta.sequence == 0) {
            homebrew->tx[ts].stream_id = dmr_homebrew_generate_stream_id();
            dmr_log_debug("homebrew: new stream on ts %d, %lu->%lu via %lu, id 0x%08lx",
                packet->ts, packet->src_id, packet->dst_id, packet->repeater_id,
                homebrew->tx[ts].stream_id);
        }
        buf[0x0f] |= (B00100000 | packet->data_type);
        break;
    }
    DMR_UINT32_BE_PACK(buf + 0x10, homebrew->tx[ts].stream_id);
    memcpy(buf + 0x14, packet->payload, DMR_PAYLOAD_BYTES);

    if (dmr_log_priority() <= DMR_LOG_PRIORITY_DEBUG) {
        dmr_homebrew_dump_data(packet);
    }
    int ret = dmr_homebrew_sendraw(homebrew, buf, sizeof(buf));
    //free(buf);
    return ret;
}

int dmr_homebrew_sendraw(dmr_homebrew_t *homebrew, uint8_t *buf, ssize_t len)
{
    if (homebrew == NULL)
        return dmr_error(DMR_EINVAL);

    dmr_log_debug("homebrew: %d bytes to %s:%d", len, inet_ntoa(homebrew->remote.sin_addr), ntohs(homebrew->remote.sin_port));
    if (dmr_log_priority() <= DMR_LOG_PRIORITY_DEBUG)
        dump_hex(buf, len);
    if (sendto(homebrew->fd, buf, len, 0, (struct sockaddr *)&homebrew->remote, sizeof(homebrew->remote)) != len) {
        dmr_log_error("homebrew: send to %s:%d failed: %s",
            inet_ntoa(homebrew->remote.sin_addr),
            ntohs(homebrew->remote.sin_port),
            strerror(errno));
        return dmr_error_set("homebrew: sendto(): %s", strerror(errno));
    }

    return 0;
}

int dmr_homebrew_recvraw(dmr_homebrew_t *homebrew, ssize_t *len, struct timeval *timeout)
{
    struct sockaddr_in peer;
    memset(&peer, 0, sizeof(struct sockaddr_in));
#ifdef DMR_PLATFORM_WINDOWS
    int peerlen;
#else
    socklen_t peerlen = sizeof(peer);
    fd_set rfds;
    int nfds;
    FD_ZERO(&rfds);
    FD_SET(homebrew->fd, &rfds);
select_again:
    nfds = select(homebrew->fd + 1, &rfds, NULL, NULL, timeout);
    if (nfds == 0) {
        dmr_log_debug("homebrew: timeout on recvraw");
        return -1;
    } else if (nfds == -1) {
        dmr_log_debug("homebrew: select: %s", strerror(errno));
        if (errno == EINTR || errno == EAGAIN) {
            goto select_again;
        }
        return dmr_error(DMR_EINVAL);
    }
    if ((*len = recvfrom(homebrew->fd, homebrew->buffer, sizeof(homebrew->buffer), 0, (struct sockaddr *)&peer, &peerlen)) < 0) {
        dmr_log_error("homebrew: recv from %s:%d failed: %s",
            inet_ntoa(homebrew->remote.sin_addr),
            ntohs(homebrew->remote.sin_port),
            strerror(errno));
        return dmr_error_set("homebrew: recvfrom(): %s", strerror(errno));
    }
#endif
    dmr_log_debug("homebrew: recv %d bytes from %s:%d", *len,
        inet_ntoa(homebrew->remote.sin_addr), ntohs(homebrew->remote.sin_port));
    if (*len > 0 && dmr_log_priority() <= DMR_LOG_PRIORITY_DEBUG)
        dump_hex(homebrew->buffer, *len);

    return 0;
}

dmr_homebrew_frame_type_t dmr_homebrew_frame_type(const uint8_t *bytes, unsigned int len)
{
    switch (len) {
    case DMR_HOMEBREW_DMR_DATA_SIZE:
        if (!memcmp(bytes, dmr_homebrew_data_signature, 4))
            return DMR_HOMEBREW_DMR_DATA_FRAME;

        break;
    default:
        break;
    }

    return DMR_HOMEBREW_UNKNOWN_FRAME;
}

/* The result from this function needs to be freed */
dmr_packet_t *dmr_homebrew_parse_packet(const uint8_t *data, unsigned int len)
{
    if (data == NULL)
        return NULL;
    if (memcmp(data, dmr_homebrew_data_signature, 4) || len != 53) {
        dmr_log_error("homebrew: can't parse packet, not a DMRD buffer");
        return NULL;
    }

    dmr_packet_t *packet = talloc_zero(NULL, dmr_packet_t);
    if (packet == NULL) {
        dmr_error(DMR_ENOMEM);
        return NULL;
    }

    packet->meta.sequence = data[4];
    packet->src_id = DMR_UINT32_BE(0, data[5], data[6], data[7]);
    packet->dst_id = DMR_UINT32_BE(0, data[8], data[9], data[10]);
    packet->repeater_id = DMR_UINT32_LE(data[11], data[12], data[13], data[14]);
    packet->ts   = (data[15] & B10000000) >> 7;
    packet->flco = (data[15] & B01000000) >> 6;
    switch        ((data[15] & B00110000) >> 4) {
    case B00000000:
        packet->data_type = DMR_DATA_TYPE_VOICE;
        packet->meta.voice_frame = (data[15] & B00001111);
        break;
    case B00000001:
        packet->data_type = DMR_DATA_TYPE_VOICE_SYNC;
        packet->meta.voice_frame = 0;
        break;
    case B00000010:
        packet->data_type = (data[15] & B00001111);
        break;
    }
    packet->meta.stream_id = DMR_UINT32_BE(data[16], data[17], data[18], data[19]);
    memcpy(packet->payload, data + 20, DMR_PAYLOAD_BYTES);

    return packet;
}
