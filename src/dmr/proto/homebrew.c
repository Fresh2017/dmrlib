#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
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
static const char dmr_homebrew_repeater_config[4]  = "RPTC";
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
        dmr_slot_type_name(packet->slot_type));

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

#ifdef DMR_DEBUG
    dmr_homebrew_data_fields_t test;
    dmr_log_trace("homebrew: dmr_homebrew_data_fields_t           size = %lu", sizeof(test));
    dmr_log_trace("homebrew: dmr_homebrew_data_fields_t.signature size = %lu", sizeof(test.signature));
    dmr_log_trace("homebrew: dmr_homebrew_data_fields_t.sequence  size = %lu", sizeof(test.sequence));
    dmr_log_trace("homebrew: dmr_homebrew_data_fields_t.dmr_data  size = %lu", sizeof(test.dmr_data));
    dmr_log_trace("homebrew: dmr_homebrew_data_fields_t.flags     size = %lu", sizeof(test.flags));
    dmr_log_trace("homebrew: dmr_homebrew_data_fields_t.stream_id size = %lu", sizeof(test.stream_id));
    assert(sizeof(dmr_homebrew_data_fields_t) == (DMR_PAYLOAD_BYTES + 20));
#endif

    int optval = 1;
    dmr_homebrew_t *homebrew = malloc(sizeof(dmr_homebrew_t));
    if (homebrew == NULL) {
        DMR_OOM();
        return NULL;
    }

    memset(homebrew, 0, sizeof(dmr_homebrew_t));
    homebrew->config = dmr_homebrew_config_new();
    if (homebrew->config == NULL) {
        DMR_OOM();
        free(homebrew);
        return NULL;
    }
    memset(homebrew->buffer, 0, sizeof(homebrew->buffer));

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
        free(homebrew->config);
        free(homebrew);
        return NULL;
    }

    // Setup file descriptor for UDP socket
    homebrew->fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (homebrew->fd < 0) {
        dmr_error_set(strerror(errno));
        free(homebrew->config);
        free(homebrew);
        return NULL;
    }
    setsockopt(homebrew->fd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval, sizeof(int));
    homebrew->server.sin_family = AF_INET;
    homebrew->server.sin_addr.s_addr = bind.s_addr;
    homebrew->server.sin_port = htons(port);
    memset(homebrew->server.sin_zero, 0, sizeof(homebrew->server.sin_zero));
    /*
    if (bind(homebrew.fd, (struct sockaddr *)&homebrew.server, sizeof(homebrew.server)) < 0) {
        fprintf(stderr, "homebrew: bind %s:%d failed: %s\n",
            inet_ntoa(homebrew.server.sin_addr), port, strerror(errno));
        return NULL;
    }
    */

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

    dmr_log_info("homebrew: connecting to repeater at %s:%d as %02x%02x%02x%02x",
        inet_ntoa(homebrew->remote.sin_addr),
        ntohs(homebrew->remote.sin_port),
        homebrew->config->repeater_id[0],
        homebrew->config->repeater_id[1],
        homebrew->config->repeater_id[2],
        homebrew->config->repeater_id[3]);

    while (homebrew->auth != DMR_HOMEBREW_AUTH_DONE) {
        switch (homebrew->auth) {
        case DMR_HOMEBREW_AUTH_NONE:
            memcpy(buf, dmr_homebrew_repeater_login, 4);
            for (i = 0, j = 0; i < 4; i++, j += 2) {
                buf[j+4] = hex[(homebrew->config->repeater_id[i] >> 4)];
                buf[j+5] = hex[(homebrew->config->repeater_id[i] & 0xf)];
            }
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
            memcpy(&buf, &homebrew->random, 8);
            memcpy(&buf[8], &secret, strlen(secret));
            sha256_init(&sha256ctx);
            sha256_update(&sha256ctx, (const uint8_t *)homebrew->random, 8);
            sha256_update(&sha256ctx, (const uint8_t *)secret, strlen(secret));
            sha256_final(&sha256ctx, &digest[0]);

            memcpy(&buf, &dmr_homebrew_repeater_key, 4);
            for (i = 0, j = 0; i < 4; i++, j += 2) {
                buf[j+4] = hex[(homebrew->config->repeater_id[i] >> 4)];
                buf[j+5] = hex[(homebrew->config->repeater_id[i] & 0xf)];
            }
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
            //memcpy(&buf, &dmr_homebrew_repeater_config, 4);
            memcpy(homebrew->config->signature, dmr_homebrew_repeater_config, 4);
#ifdef DMR_DEBUG
            assert(sizeof(dmr_homebrew_config_t) == 306);
#endif
            memcpy(buf, homebrew->config, sizeof(dmr_homebrew_config_t));
            /*
            uint16_t pos = 4;
#define C(attr) do { memcpy(buf + pos, attr, sizeof(attr)); pos += sizeof(attr); } while(0)
            C(homebrew->config->callsign);
            for (i = 0; i < 4; i++) {
                buf[pos++] = hex[(homebrew->config->repeater_id[i] >> 4)];
                buf[pos++] = hex[(homebrew->config->repeater_id[i] & 0xf)];
            }
            C(homebrew->config->rx_freq);
            C(homebrew->config->tx_freq);
            C(homebrew->config->tx_power);
            C(homebrew->config->color_code);
            C(homebrew->config->latitude);
            C(homebrew->config->longitude);
            C(homebrew->config->height);
            C(homebrew->config->location);
            C(homebrew->config->description);
            C(homebrew->config->url);
            C(homebrew->config->software_id);
            C(homebrew->config->package_id);
#undef C
            dmr_log_trace("homebrew: sending %d bytes of config", pos);
#ifdef DMR_DEBUG
            dump_hex(&buf[4], pos - 4);
            //assert(pos == 302);
#endif
            */
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
    int i, j;

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
    for (i = 0, j = 0; i < 4; i++, j += 2) {
        buf[j+4] = hex[(homebrew->config->repeater_id[i] >> 4)];
        buf[j+5] = hex[(homebrew->config->repeater_id[i] & 0xf)];
    }
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

    free(homebrew->config);
    free(homebrew);
}

void dmr_homebrew_dump_data(uint8_t *buf, dmr_packet_t *packet)
{
    /*
    uint8_t    ts  = (buf[5])      & 0x01U,
        call_type  = (buf[5] >> 1) & 0x01U,
        frame_type = (buf[5] >> 2) & 0x03U,
        data_type  = (buf[5] >> 4) & 0x07U;
    uint16_t stream_id = (buf[16] << 24) | (buf[17] << 16) | (buf[18] << 8) | buf[19];
    */
    if (buf == NULL || packet == NULL)
        return;

    dmr_homebrew_data_fields_t *data = (dmr_homebrew_data_fields_t *)buf;
    dmr_log_debug("homebrew: frame [%lu->%lu] via %lu, seq 0x%02x, slot %d, call type %d, frame type %d, data type %d, stream id 0x%08lx:",
        packet->src_id,
        packet->dst_id,
        packet->repeater_id,
        data->sequence,
        data->flags.slot,
        data->flags.call_type,
        data->flags.frame_type,
        data->flags.data_type,
        data->stream_id);
    dmr_log_debug("homebrew:  signature: %.s", 4, buf);
    dmr_log_debug("homebrew:   sequence: 0x%02x", buf[4]);
    dmr_log_debug("homebrew:             0b%s",      dmr_byte_to_binary(buf[16]));
    dmr_log_debug("homebrew:       flco: 0b%s",      dmr_byte_to_binary(buf[16] & 0x0fU));
    dmr_log_debug("homebrew:         ts: 0b%s (%d)", dmr_byte_to_binary(buf[16] & B00000001), data->flags.slot);
    dmr_log_debug("homebrew:  call type: 0b%s (%d)", dmr_byte_to_binary(buf[16] & B00000010), data->flags.call_type);
    dmr_log_debug("homebrew: frame type: 0b%s (%d)", dmr_byte_to_binary(buf[16] & B00001100), data->flags.frame_type);
    dmr_log_debug("homebrew:  data type: 0b%s (%d)", dmr_byte_to_binary(buf[16] & B11110000), data->flags.data_type);
    dmr_log_debug("homebrew:  stream id: 0x%08lx", data->stream_id);
    dump_hex(buf, DMR_PAYLOAD_BYTES + 20);
}

void dmr_homebrew_loop(dmr_homebrew_t *homebrew)
{
    uint8_t buf[53], i, j;
    ssize_t len = 0, pos;
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
            for (i = 0, j = 0; i < 4; i++, j += 2) {
                buf[j+7] = hex[(homebrew->config->repeater_id[i] >> 4)];
                buf[j+8] = hex[(homebrew->config->repeater_id[i] & 0xf)];
            }
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
        } else if (len > 4 && !memcmp(homebrew->buffer, dmr_homebrew_data_signature, 4)) {
            dmr_log_debug("homebrew: got data packet");
            dmr_packet_t *packet = malloc(sizeof(dmr_packet_t));
            if (packet == NULL) {
                dmr_error(DMR_ENOMEM);
                return;
            }
            memset(packet, 0, sizeof(dmr_packet_t));
            pos = 4 + 1; /* 'D', 'M', 'R', 'D', seqno */
            packet->src_id |= (homebrew->buffer[pos++] << 16);
            packet->src_id |= (homebrew->buffer[pos++] << 8);
            packet->src_id |= (homebrew->buffer[pos++] << 0);
            packet->dst_id |= (homebrew->buffer[pos++] << 16);
            packet->dst_id |= (homebrew->buffer[pos++] << 8);
            packet->dst_id |= (homebrew->buffer[pos++] << 0);
            packet->repeater_id |= (homebrew->buffer[pos++] << 0);
            packet->repeater_id |= (homebrew->buffer[pos++] << 8);
            packet->repeater_id |= (homebrew->buffer[pos++] << 16);
            packet->repeater_id |= (homebrew->buffer[pos++] << 24);
            uint8_t data_type = homebrew->buffer[pos++];
            switch ((data_type >> 2) & B00000011) {
            case B00000000: /* voice */
                packet->slot_type = DMR_SLOT_TYPE_VOICE_BURST_A + (data_type >> 4);
                break;
            case B00000001: /* voice sync */
                packet->slot_type = DMR_SLOT_TYPE_VOICE_BURST_A;
                break;
            case B00000010: /* data sync */
            default:
                packet->slot_type = (data_type >> 4) & B00001111;
                break;
            }
            pos += 4; /* stream id */
            memcpy(packet->payload, &homebrew->buffer[pos], DMR_PAYLOAD_BYTES);
            if (dmr_log_priority() <= DMR_LOG_PRIORITY_DEBUG) {
                dump_hex(packet->payload, DMR_PAYLOAD_BYTES);
            }
            dmr_homebrew_dump_data(homebrew->buffer, packet);
            homebrew->proto.rx(homebrew, packet);
            free(packet);
        } else if (len > 7 && !memcmp(homebrew->buffer, dmr_homebrew_repeater_pong, 7)) {
            dmr_log_debug("homebrew: master sent pong");
        } else if (len > 7 && !memcmp(homebrew->buffer, dmr_homebrew_repeater_beacon, 7)) {
            dmr_log_debug("homebrew: master sent synchronous site beacon (ignored)");
        } else if (len > 7 && !memcmp(homebrew->buffer, dmr_homebrew_repeater_rssi, 7)) {
            dmr_log_debug("homebrew: master sent repeater RSSI (ignored)");
        } else if (len > 6 && !memcmp(homebrew->buffer, dmr_homebrew_master_ack, 6)) {
            dmr_log_debug("homebrew: master sent ack");
        } else if (len > 5 && !memcmp(homebrew->buffer, dmr_homebrew_master_closing, 5)) {
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
    //uint8_t *buf, pos = 0;

    if (homebrew == NULL || packet == NULL || ts > DMR_TS2)
        return dmr_error(DMR_EINVAL);

    /*
    buf = malloc(DMR_PAYLOAD_BYTES + 20);
    if (buf == NULL)
        return dmr_error(DMR_ENOMEM);

    memset(buf, 0, DMR_PAYLOAD_BYTES + 20);
    */
    uint8_t buf[DMR_PAYLOAD_BYTES + 20], pos = 0;

    if (packet->repeater_id == 0)
        packet->repeater_id = homebrew->id;

    bool reset = false;
#define C(field) do { if (homebrew->tx[ts].field != (*packet).field) { dmr_log_debug("homebrew: stream 0x%08lx expired: " #field " changed", homebrew->tx[ts].stream_id); reset = true; }} while(0)
    C(src_id);
    C(dst_id);
    C(call_type);
#undef C
    double delta = dmr_time_ms_since(homebrew->tx[ts].last_voice_packet_sent);
    if (delta > 200.0) {
        dmr_log_debug("homebrew: stream 0x%08lx expired: delta %.2f > 200.0",
            homebrew->tx[ts].stream_id, delta);
        reset = true;
    }

    if (reset) {
        homebrew->tx[ts].src_id      = packet->src_id;
        homebrew->tx[ts].dst_id      = packet->dst_id;
        homebrew->tx[ts].call_type   = packet->call_type;
        homebrew->tx[ts].slot_type   = packet->slot_type;
        homebrew->tx[ts].seq         = 0;
        homebrew->tx[ts].stream_id   = (rand() % 0xffU);
        homebrew->tx[ts].stream_id  |= (rand() % 0xffU) << 8;
        homebrew->tx[ts].stream_id  |= (rand() % 0xffU) << 16;
        homebrew->tx[ts].stream_id  |= (rand() % 0xffU) << 24;
        dmr_log_debug("homebrew: new stream on ts %d, %lu->%lu via %lu, id 0x%08lx",
            packet->ts, packet->src_id, packet->dst_id, packet->repeater_id,
            homebrew->tx[ts].stream_id);
    }

    memcpy(buf, dmr_homebrew_data_signature, 4);
    pos = 4;
    buf[pos++] = homebrew->tx[ts].seq++ % 0xffU;
    buf[pos++] = (packet->src_id >> 16);
    buf[pos++] = (packet->src_id >> 8);
    buf[pos++] = (packet->src_id >> 0);
    buf[pos++] = (packet->dst_id >> 16);
    buf[pos++] = (packet->dst_id >> 8);
    buf[pos++] = (packet->dst_id >> 0);
    buf[pos++] = (packet->repeater_id >> 0);
    buf[pos++] = (packet->repeater_id >> 8);
    buf[pos++] = (packet->repeater_id >> 16);
    buf[pos++] = (packet->repeater_id >> 24);
    buf[pos] = ((ts)                                   & B00000001) |
               ((packet->call_type << 1)               & B00000010);

    switch (packet->slot_type) {
    case DMR_SLOT_TYPE_VOICE_BURST_A:
        buf[pos] |= (0x00 << 2)                        & B00001100;
        buf[pos] |= ((packet->slot_type - 0x0aU) << 4) & B11110000;
        gettimeofday(&homebrew->tx[ts].last_voice_packet_sent, NULL);
        break;
    case DMR_SLOT_TYPE_VOICE_BURST_B:
    case DMR_SLOT_TYPE_VOICE_BURST_C:
    case DMR_SLOT_TYPE_VOICE_BURST_D:
    case DMR_SLOT_TYPE_VOICE_BURST_E:
    case DMR_SLOT_TYPE_VOICE_BURST_F:
        buf[pos] |= (0x01 << 2)                        & B00001100;
        buf[pos] |= ((packet->slot_type - 0x0aU) << 4) & B11110000;
        gettimeofday(&homebrew->tx[ts].last_voice_packet_sent, NULL);
        break;
    default:
        buf[pos] |= (0x02 << 2)                        & B00001100;
        buf[pos] |= ((packet->slot_type & 0x0f) << 4)  & B11110000;
        gettimeofday(&homebrew->tx[ts].last_data_packet_sent, NULL);
        break;
    }
    pos++;

    buf[pos++] = (homebrew->tx[ts].stream_id >> 24) & 0xffU;
    buf[pos++] = (homebrew->tx[ts].stream_id >> 16) & 0xffU;
    buf[pos++] = (homebrew->tx[ts].stream_id >>  8) & 0xffU;
    buf[pos++] = (homebrew->tx[ts].stream_id >>  0) & 0xffU;
#ifdef DMR_DEBUG
    assert(pos == 20);
#endif
    memcpy(&buf[pos], packet->payload, DMR_PAYLOAD_BYTES);
    if (dmr_log_priority() <= DMR_LOG_PRIORITY_DEBUG) {
        dmr_homebrew_dump_data(buf, packet);
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
dmr_homebrew_packet_t *dmr_homebrew_parse_packet(const uint8_t *bytes, unsigned int len)
{
    if (bytes == NULL)
        return NULL;
    if (dmr_homebrew_frame_type(bytes, len) != DMR_HOMEBREW_DMR_DATA_FRAME)
        return NULL;

    dmr_homebrew_packet_t *packet = malloc(sizeof(dmr_homebrew_packet_t));
    if (packet == NULL) {
        dmr_error(DMR_ENOMEM);
        return NULL;
    }

    memset(packet, 0, sizeof(dmr_homebrew_packet_t));
    memcpy(&packet->data, bytes, sizeof(dmr_homebrew_data_fields_t));
    /*
    memcpy(packet->fields.flags.signature, bytes, 4);
    packet->fields.flags.sequence = bytes[4];
    packet->packet.src_id = (bytes[5] << 16) | (bytes[6] << 8) | bytes[7];
    packet->packet.dst_id = (bytes[8] << 16) | (bytes[9] << 8) | bytes[10];
    packet->packet.repeater_id  = (bytes[11] << 24);
    packet->packet.repeater_id |= (bytes[12] << 16);
    packet->packet.repeater_id |= (bytes[13] <<  8);
    packet->packet.repeater_id |= (bytes[14] <<  0);
    packet->fields.flags.byte = bytes[15];
    packet->fields.src_id = packet->packet.src_id;
    packet->fields.dst_id = packet->packet.dst_id;
    packet->fields.repeater_id = packet->packet.repeater_id;
    */
    /*
    packet->packet.ts = (bytes[15]) & 0x01;
    packet->fields.flags.call_type = (bytes[15] >> 1) & 0x01;
    packet->dmr_packet.call_type = packet->call_type;
    packet->frame_type = (bytes[15] >> 2) & 0x03;
    packet->data_type = (bytes[15] >> 4) & 0x0f;
    */
    switch (packet->data.fields.flags.frame_type) {
    case DMR_HOMEBREW_DATA_TYPE_VOICE:
    case DMR_HOMEBREW_DATA_TYPE_VOICE_SYNC:
        packet->packet.slot_type = packet->data.fields.flags.data_type + DMR_SLOT_TYPE_VOICE_BURST_A;
        break;
    case DMR_HOMEBREW_DATA_TYPE_DATA_SYNC: // data sync
        packet->packet.slot_type = packet->data.fields.flags.data_type;
        break;
    default:
        packet->packet.slot_type = DMR_SLOT_TYPE_UNKNOWN;
    }

    memcpy(&packet->packet.payload, &bytes[20], DMR_PAYLOAD_BYTES);
    /*
    memcpy(&packet->fields.dmr_data, &bytes[20], DMR_PAYLOAD_BYTES);
    */

    return packet;
}
