#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <talloc.h>
#ifdef DMR_DEBUG
#include <assert.h>
#endif
#if defined(DMR_PLATFORM_LINUX)
#include <sys/socket.h>
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

static struct {
    dmr_homebrew_frame_type_t frame_type;
    char                      *name;
} dmr_homebrew_frame_type_names[] = {
    { DMR_HOMEBREW_DMR_DATA_FRAME,   "DMR data"              },
    { DMR_HOMEBREW_MASTER_ACK,       "master ack"            },
    { DMR_HOMEBREW_MASTER_ACK_NONCE, "master ack with nonce" },
    { DMR_HOMEBREW_MASTER_CLOSING,   "master closing"        },
    { DMR_HOMEBREW_MASTER_NAK,       "master nak"            },
    { DMR_HOMEBREW_MASTER_PING,      "master ping"           },
    { DMR_HOMEBREW_REPEATER_BEACON,  "repeater beacon"       },
    { DMR_HOMEBREW_REPEATER_CLOSING, "repeater closing"      },
    { DMR_HOMEBREW_REPEATER_KEY,     "repeater key"          },
    { DMR_HOMEBREW_REPEATER_LOGIN,   "repeater login"        },
    { DMR_HOMEBREW_REPEATER_PONG,    "repeater pong"         },
    { DMR_HOMEBREW_REPEATER_RSSI,    "repeater RSSI"         },
    { DMR_HOMEBREW_UNKNOWN,          "unkonwn"               },
    { 0, NULL } /* sentinel */
};

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

    dmr_thread_name_set("homebrew");
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
    dmr_ts_t ts;
    for (ts = DMR_TS1; ts < DMR_TS_INVALID; ts++) {
        packet->ts = ts;
        if (dmr_homebrew_send(homebrew, packet->ts, packet) != 0) {
            dmr_log_error("homebrew: send failed: %s", dmr_error_get());
            return;
        }
    }
}

dmr_homebrew_t *dmr_homebrew_new(int port, struct in_addr peer)
{
    dmr_log_debug("homebrew: new on port %d to %s:%d",
        port, inet_ntoa(peer), port);

    int optval = 1;
    dmr_homebrew_t *homebrew = talloc_zero(NULL, dmr_homebrew_t);
    if (homebrew == NULL) {
        DMR_OOM();
        return NULL;
    }

    dmr_homebrew_config_init(&homebrew->config);

    dmr_ts_t ts;
    for (ts = DMR_TS1; ts < DMR_TS_INVALID; ts++) {
        memset(&homebrew->tx[ts].last_voice_packet_sent, 0, sizeof(struct timeval));
        memset(&homebrew->tx[ts].last_data_packet_sent, 0, sizeof(struct timeval));
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
        TALLOC_FREE(homebrew);
        return NULL;
    }

    // Setup send queue
    homebrew->queue_size = 32;
    homebrew->queue_used = 0;
    homebrew->queue_lock = talloc_zero(homebrew, dmr_mutex_t);
    homebrew->queue = talloc_size(homebrew, sizeof(dmr_packet_t) * homebrew->queue_size);

    if (homebrew->queue == NULL) {
        dmr_log_critical("homebrew: out of memory");
        TALLOC_FREE(homebrew);
        return NULL;
    }

    // Setup file descriptor for UDP socket
    homebrew->fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (homebrew->fd < 0) {
        dmr_error_set(strerror(errno));
        dmr_log_error("homebrew: socket creation failed: %s", strerror(errno));
        TALLOC_FREE(homebrew);
        return NULL;
    }

#if defined(SO_REUSEADDR)
    if (setsockopt(homebrew->fd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval, sizeof(int)) != 0) {
        dmr_log_error("homebrew: failed to set socket option SO_REUSEADDR: %s", strerror(errno));
    }
#endif
#if defined(SO_REUSEPORT)
    if (setsockopt(homebrew->fd, SOL_SOCKET, SO_REUSEPORT, (const void *)&optval, sizeof(int)) != 0) {
        dmr_log_error("homebrew: failed to set socket option SO_REUSEPORT: %s", strerror(errno));
    }
#endif

    homebrew->server.sin_family = AF_INET;
    homebrew->server.sin_addr.s_addr = INADDR_ANY;
    homebrew->server.sin_port = htons(port);
    memset(homebrew->server.sin_zero, 0, sizeof(homebrew->server.sin_zero));

    if (bind(homebrew->fd, (struct sockaddr *)&homebrew->server, sizeof(struct sockaddr_in)) != 0) {
        dmr_error_set(strerror(errno));
        dmr_log_error("homebrew: bind to port %d failed: %s", port, strerror(errno));
        TALLOC_FREE(homebrew);
        return NULL;
    }

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

    dmr_log_info("homebrew: connecting to repeater at %s:%d as %.*s",
        inet_ntoa(homebrew->remote.sin_addr),
        ntohs(homebrew->remote.sin_port),
        /* no NULL byte at the end */
        8, homebrew->config.repeater_id);

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

/** Add a packet to the send queue. */
int dmr_homebrew_queue(dmr_homebrew_t *homebrew, dmr_packet_t *packet_out)
{
    if (homebrew == NULL || packet_out == NULL)
        return dmr_error(DMR_EINVAL);

    //dmr_mutex_lock(homebrew->queue_lock);
    int ret = 0;
    if (homebrew->queue_used + 1 < homebrew->queue_size) {
        dmr_packet_t *packet = talloc_zero(NULL, dmr_packet_t);
        if (packet == NULL) {
            dmr_log_critical("homebrew: out of memory");
            return dmr_error(DMR_ENOMEM);
        }
        memcpy(packet, packet_out, sizeof(dmr_packet_t));
        homebrew->queue[homebrew->queue_used++] = packet;
    } else {
        dmr_log_error("homebrew: send queue full!");
        ret = -1;
    }
    //dmr_mutex_unlock(homebrew->queue_lock);

    return ret;
}

dmr_packet_t *dmr_homebrew_queue_shift(dmr_homebrew_t *homebrew)
{
    if (homebrew == NULL)
        return NULL;

    dmr_packet_t *packet = NULL;
    //dmr_mutex_lock(homebrew->queue_lock);
    if (homebrew->queue_used) {
        packet = homebrew->queue[0];
        homebrew->queue_used--;
        size_t i;
        for (i = 1; i < homebrew->queue_used; i++) {
            homebrew->queue[i - 1] = homebrew->queue[i];
        }
    }
    //dmr_mutex_unlock(homebrew->queue_lock);

    return packet;
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

    TALLOC_FREE(homebrew);
}

char *dmr_homebrew_frame_type_name(dmr_homebrew_frame_type_t frame_type)
{
    int i;
    for (i = 0; dmr_homebrew_frame_type_names[i].name != NULL; i++) {
        if (frame_type == dmr_homebrew_frame_type_names[i].frame_type) {
            return dmr_homebrew_frame_type_names[i].name;
        }
    }

    return "unkonwn";
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

        /*
        dmr_packet_t *out = dmr_homebrew_queue_shift(homebrew);
        if (out != NULL) {
            dmr_ts_t ts;
            for (ts = DMR_TS1; ts < DMR_TS_INVALID; ts++) {
                out->ts = ts;
                if ((ret = dmr_homebrew_send(homebrew, out->ts, out)) != 0) {
                    dmr_log_error("homebrew: send failed: %s", dmr_error_get());
                    return;
                }
            }
            //TALLOC_FREE(out);
        }
        */

        if ((ret = dmr_homebrew_recvraw(homebrew, &len, &timeout)) != 0) {
            // Only log errors if it's not a timeout
            if (ret == -1)
                continue;

            dmr_log_error("homebrew: loop error: %s", dmr_error_get());
            break;
        }

        dmr_homebrew_frame_type_t frame_type = dmr_homebrew_frame_type(homebrew->buffer, len);
        switch (frame_type) {
        case DMR_HOMEBREW_MASTER_PING:
            dmr_log_debug("homebrew: ping? pong!");
            memcpy(buf, homebrew->buffer, 15);
            memcpy(buf, dmr_homebrew_repeater_pong, 7);
            if (!dmr_homebrew_sendraw(homebrew, buf, 15))
                return;

            break;

        case DMR_HOMEBREW_DMR_DATA_FRAME:
            dmr_log_debug("homebrew: got data packet");
            dmr_packet_t *packet = dmr_homebrew_parse_packet(homebrew->buffer, len);
            if (packet == NULL) {
                dmr_error(DMR_ENOMEM);
                return;
            }
            /*
            if (dmr_log_priority() <= DMR_LOG_PRIORITY_DEBUG) {
                dmr_dump_hex(packet->payload, DMR_PAYLOAD_BYTES);
            }
            */

            /* Not sure why this happens, but on TG dupe streams are sent, one with the
            * stream_id set to the dst_id */
            if ((packet->dst_id < 0xff && packet->dst_id == packet->meta.stream_id) || packet->dst_id == 204) {
                dmr_log_debug("homebrew: ignored stream 0x%08x: bogus", packet->meta.stream_id);
                TALLOC_FREE(packet);
                continue;
            }

            dmr_dump_packet(packet);
            homebrew->proto.rx(homebrew, packet);
            TALLOC_FREE(packet);

            break;

        case DMR_HOMEBREW_REPEATER_PONG:
            dmr_log_debug("homebrew: master sent pong");
            break;

        case DMR_HOMEBREW_REPEATER_BEACON:
            dmr_log_debug("homebrew: master sent synchronous site beacon (ignored)");
            break;

        case DMR_HOMEBREW_REPEATER_RSSI:
            dmr_log_debug("homebrew: master sent repeater RSSI (ignored)");
            break;

        case DMR_HOMEBREW_MASTER_ACK:
            dmr_log_debug("homebrew: master sent ack");
            break;

        case DMR_HOMEBREW_MASTER_CLOSING:
            dmr_log_critical("homebrew: master closing");
            break;

        default:
            dmr_log_debug("homebrew: master sent %s", dmr_homebrew_frame_type_name(frame_type));
            break;
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

    /* Reset the stream if this is a *new* voice LC */
    if (packet->data_type == DMR_DATA_TYPE_VOICE_LC &&
        homebrew->tx[ts].data_type != DMR_DATA_TYPE_VOICE_LC) {
        homebrew->tx[ts].stream_id++;
        homebrew->tx[ts].seq = 0;
        dmr_log_debug("homebrew: new stream on %s, %lu->%lu via %lu, id 0x%08lx",
            dmr_ts_name(ts), packet->src_id, packet->dst_id, packet->repeater_id,
            homebrew->tx[ts].stream_id);
    }
    packet->meta.stream_id = homebrew->tx[ts].stream_id;
    packet->meta.sequence = homebrew->tx[ts].seq++;

    dmr_homebrew_data_t dmrd;
    memcpy(&dmrd.signature, "DMRD", 4);
    dmrd.sequence = packet->meta.sequence;
    dmrd.stream_id[3] = packet->meta.stream_id >> 24;
    dmrd.stream_id[2] = packet->meta.stream_id >> 16;
    dmrd.stream_id[1] = packet->meta.stream_id >>  8;
    dmrd.stream_id[0] = packet->meta.stream_id >>  0;
    dmrd.src_id[0] = packet->src_id >> 16;
    dmrd.src_id[1] = packet->src_id >>  8;
    dmrd.src_id[2] = packet->src_id >>  0;
    dmrd.dst_id[0] = packet->dst_id >> 16;
    dmrd.dst_id[1] = packet->dst_id >>  8;
    dmrd.dst_id[2] = packet->dst_id >>  0;
    dmrd.repeater_id[0] = packet->repeater_id >> 24;
    dmrd.repeater_id[1] = packet->repeater_id >> 16;
    dmrd.repeater_id[2] = packet->repeater_id >>  8;
    dmrd.repeater_id[3] = packet->repeater_id >>  0;
    dmrd.type = (packet->ts & 0x01) | (packet->flco & 0x01) << 1;
    switch (packet->data_type) {
    case DMR_DATA_TYPE_VOICE:
        dmrd.type |= (packet->meta.voice_frame & 0x0f) << 4;
        break;
    case DMR_DATA_TYPE_VOICE_SYNC:
        dmrd.type |= 0x04;
        break;
    default:
        dmrd.type |= 0x0b;
        dmrd.type |= (packet->data_type & 0x0f) << 4;
        break;
    }
    memcpy(&dmrd.payload, packet->payload, DMR_PAYLOAD_BYTES);

    if (dmr_log_priority() <= DMR_LOG_PRIORITY_DEBUG) {
        dmr_dump_packet(packet);
    }
    int ret = dmr_homebrew_sendraw(homebrew, (uint8_t *)&dmrd, sizeof(dmr_homebrew_data_t));

    /* Book keeping */
    homebrew->tx[ts].data_type = packet->data_type;

    //free(buf);
    return ret;
}

int dmr_homebrew_sendraw(dmr_homebrew_t *homebrew, uint8_t *buf, ssize_t len)
{
    if (homebrew == NULL)
        return dmr_error(DMR_EINVAL);

    dmr_log_debug("homebrew: %d bytes to %s:%d", len, inet_ntoa(homebrew->remote.sin_addr), ntohs(homebrew->remote.sin_port));
    if (dmr_log_priority() <= DMR_LOG_PRIORITY_DEBUG) {
        dmr_dump_hex(buf, len);
        dmr_homebrew_dump(buf, len);
    }
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
    if (*len > 0 && dmr_log_priority() <= DMR_LOG_PRIORITY_DEBUG) {
        dmr_dump_hex(homebrew->buffer, *len);
        dmr_homebrew_dump(homebrew->buffer, *len);
    }
    return 0;
}

dmr_homebrew_frame_type_t dmr_homebrew_frame_type(const uint8_t *bytes, ssize_t len)
{
    switch (len) {
    case 12:
        if (!memcmp(bytes, dmr_homebrew_repeater_login, 4))
            return DMR_HOMEBREW_REPEATER_LOGIN;

        break;

    case 13:
        if (!memcmp(bytes, dmr_homebrew_master_closing, 5))
            return DMR_HOMEBREW_MASTER_CLOSING;
        if (!memcmp(bytes, dmr_homebrew_repeater_closing, 5))
            return DMR_HOMEBREW_REPEATER_CLOSING;

        break;

    case 14:
        if (!memcmp(bytes, dmr_homebrew_master_ack, 6))
            return DMR_HOMEBREW_MASTER_ACK;
        if (!memcmp(bytes, dmr_homebrew_master_nak, 6))
            return DMR_HOMEBREW_MASTER_NAK;

        break;

    case 15:
        if (!memcmp(bytes, dmr_homebrew_master_ping, 7))
            return DMR_HOMEBREW_MASTER_PING;
        if (!memcmp(bytes, dmr_homebrew_repeater_pong, 7))
            return DMR_HOMEBREW_REPEATER_PONG;
        if (!memcmp(bytes, dmr_homebrew_repeater_beacon, 7))
            return DMR_HOMEBREW_REPEATER_BEACON;

        break;

    case 22:
        if (!memcmp(bytes, dmr_homebrew_master_ack, 6))
            return DMR_HOMEBREW_MASTER_ACK_NONCE;

        break;

    case 23:
        if (!memcmp(bytes, dmr_homebrew_repeater_rssi, 7))
            return DMR_HOMEBREW_REPEATER_RSSI;

        break;

    case 53:
        if (!memcmp(bytes, dmr_homebrew_data_signature, 4))
            return DMR_HOMEBREW_DMR_DATA_FRAME;

        break;

    case 76:
        if (!memcmp(bytes, dmr_homebrew_repeater_key, 4))
            return DMR_HOMEBREW_REPEATER_KEY;

    default:
        break;
    }

    return DMR_HOMEBREW_UNKNOWN;
}

dmr_homebrew_frame_type_t dmr_homebrew_dump(uint8_t *buf, ssize_t len)
{
    if (buf == NULL || len == 0)
        return 0;

    dmr_homebrew_frame_type_t frame_type = dmr_homebrew_frame_type(buf, len);
    dmr_log_debug("homebrew: %zu bytes of %s", len, dmr_homebrew_frame_type_name(frame_type));
    //dmr_dump_hex(buf, len);
    switch (frame_type) {
    case DMR_HOMEBREW_DMR_DATA_FRAME:
        if (dmr_log_priority() <= DMR_LOG_PRIORITY_DEBUG) {
            dmr_homebrew_data_t *dmrd = (dmr_homebrew_data_t *)buf;
            dmr_id_t src_id = (dmrd->src_id[0] << 16) | (dmrd->src_id[1] << 8) | dmrd->src_id[2];
            dmr_id_t dst_id = (dmrd->dst_id[0] << 16) | (dmrd->dst_id[1] << 8) | dmrd->dst_id[2];
            dmr_id_t repeater_id = (dmrd->repeater_id[0] << 24) | (dmrd->repeater_id[1] << 16) | (dmrd->repeater_id[2] << 8) | dmrd->repeater_id[3];
            uint32_t stream_id = (dmrd->stream_id[3] << 24) | (dmrd->stream_id[2] << 16) | (dmrd->stream_id[1] << 8) | dmrd->stream_id[0];   
            dmr_log_debug("homebrew: sequence: %u (0x%02x)", dmrd->sequence, dmrd->sequence);
            dmr_log_debug("homebrew: src->dst: %u->%u", src_id, dst_id);
            dmr_log_debug("homebrew: repeater: %u (%08x)", repeater_id, repeater_id);
            dmr_log_debug("homebrew:     type: %s", dmr_byte_to_binary(dmrd->type));
            dmr_log_debug("homebrew:       ts: %d", (dmrd->type & 0x01));
            dmr_log_debug("homebrew:     flco: %d", (dmrd->type & 0x02) >> 1);
            dmr_log_debug("homebrew:     type: %d", (dmrd->type & 0x0c) >> 2);
            dmr_log_debug("homebrew: streamid: %u (%08x)", stream_id, stream_id);
            switch ((dmrd->type & 0x0c) >> 2) {
            case 0x00:
                {
                    uint8_t frame = (dmrd->type & 0xf0) >> 4;
                    dmr_log_debug("homebrew:     data: voice frame %c (%d)",
                        'A' + frame, frame);
                    break;
                }
            case 0x01:
                dmr_log_debug("homebrew:     data: voice sync");
                break;
            case 0x02:
                dmr_log_debug("homebrew:     data: %s (%d)",
                    dmr_data_type_name((dmrd->type & 0xf0) >> 4), (dmrd->type & 0xf0) >> 4);
                break;
            }
        }

        break;
    default:
        break;
    }

    return frame_type;
}

/* The result from this function needs to be freed */
dmr_packet_t *dmr_homebrew_parse_packet(const uint8_t *data, ssize_t len)
{
    if (data == NULL) {
        dmr_error(DMR_EINVAL);
        return NULL;
    }

    if (dmr_homebrew_frame_type(data, len) != DMR_HOMEBREW_DMR_DATA_FRAME) {
        dmr_log_error("homebrew: can't parse packet, not a DMRD buffer");
        return NULL;
    }

    dmr_packet_t *packet = talloc_zero(NULL, dmr_packet_t);
    if (packet == NULL) {
        dmr_error(DMR_ENOMEM);
        return NULL;
    }
    dmr_homebrew_data_t *dmrd = (dmr_homebrew_data_t *)data;

    assert(sizeof(dmr_homebrew_data_t) == 53);

    packet->meta.sequence = dmrd->sequence;
    packet->meta.stream_id = (dmrd->stream_id[3] << 24) | (dmrd->stream_id[2] << 16) | (dmrd->stream_id[1] << 8) | dmrd->stream_id[0];
    packet->repeater_id = (dmrd->repeater_id[0] << 24) | (dmrd->repeater_id[1] << 16) | (dmrd->repeater_id[2] << 8) | dmrd->repeater_id[3];
    packet->src_id = (dmrd->src_id[0] << 16) | (dmrd->src_id[1] << 8) | dmrd->src_id[2];
    packet->dst_id = (dmrd->dst_id[0] << 16) | (dmrd->dst_id[1] << 8) | dmrd->dst_id[2];
    packet->ts = (dmrd->type & 0x01);
    packet->flco = (dmrd->type & 0x02) >> 1;
    switch ((dmrd->type & 0x0c) >> 2) {
    case 0x00:
        packet->data_type = DMR_DATA_TYPE_VOICE;
        packet->meta.voice_frame = (dmrd->type & 0xf0) >> 4;
        break;
    case 0x01:
        packet->data_type = DMR_DATA_TYPE_VOICE_SYNC;
        packet->meta.voice_frame = 0;
        break;
    case 0x02:
        packet->data_type = (dmrd->type & 0xf0) >> 4;
        break;
    }
    memcpy(packet->payload, dmrd->payload, DMR_PAYLOAD_BYTES);

    return packet;
}
