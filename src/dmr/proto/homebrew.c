#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
//#include <openssl/sha.h>

#include "dmr/proto.h"
#include "dmr/proto/homebrew.h"
#include "dmr/type.h"
#include "sha256.h"

const char *dmr_homebrew_proto_name = "homebrew";
const char hex[16] = "0123456789abcdef";

const char dmr_homebrew_data_signature[4]   = "DMRD";
const char dmr_homebrew_master_ack[6]       = "MSTACK";
const char dmr_homebrew_master_nak[6]       = "MSTNAK";
const char dmr_homebrew_master_ping[7]      = "MSTPING";
const char dmr_homebrew_master_closing[5]   = "MSTCL";
const char dmr_homebrew_repeater_login[4]   = "RPTL";
const char dmr_homebrew_repeater_key[4]     = "RPTK";
const char dmr_homebrew_repeater_pong[7]    = "RPTPONG";
const char dmr_homebrew_repeater_closing[5] = "RPTCL";
const char dmr_homebrew_repeater_config[4]  = "RPTC";


static dmr_proto_status_t homebrew_proto_init(void *homebrewptr)
{
    dmr_homebrew_t *homebrew = (dmr_homebrew_t *)homebrewptr;
    if (homebrew == NULL)
        return DMR_PROTO_CONF;
    else if (homebrew->auth != DMR_HOMEBREW_AUTH_DONE)
        return DMR_PROTO_NOT_READY;
    else
        return DMR_PROTO_OK;
}

static int homebrew_proto_start_thread(void *homebrewptr)
{
    dmr_homebrew_t *homebrew = (dmr_homebrew_t *)homebrewptr;
    if (homebrew == NULL)
        return dmr_thread_error;

    dmr_homebrew_loop(homebrew);
    return dmr_thread_success;
}

static bool homebrew_proto_start(void *homebrewptr)
{
    int ret;
    dmr_homebrew_t *homebrew = (dmr_homebrew_t *)homebrewptr;
    if (homebrew == NULL)
        return false;

    if (homebrew->thread != NULL) {
        fprintf(stderr, "homebrew: can't start, already active\n");
        return false;
    }

    homebrew->thread = malloc(sizeof(dmr_thread_t));
    if (homebrew->thread == NULL) {
        fprintf(stderr, "homebrew: can't start, out of memory\n");
        return false;
    }

    switch ((ret = dmr_thread_create(homebrew->thread, homebrew_proto_start_thread, homebrewptr))) {
    case dmr_thread_success:
        break;
    default:
        fprintf(stderr, "homebrew: can't create thread\n");
        return false;
    }

    return true;
}

static bool homebrew_proto_stop(void *homebrewptr)
{
    int ret;
    dmr_homebrew_t *homebrew = (dmr_homebrew_t *)homebrewptr;
    if (homebrew == NULL)
        return false;

    if (homebrew->thread == NULL) {
        fprintf(stderr, "homebrew: not active\n");
        return true;
    }

    homebrew->run = false;
    switch ((ret = dmr_thread_join(*homebrew->thread, NULL))) {
    case dmr_thread_success:
        break;
    default:
        fprintf(stderr, "homebrew: can't stop thread\n");
        return false;
    }

    free(homebrew->thread);
    homebrew->thread = NULL;

    dmr_homebrew_close(homebrew);
    return true;
}

static bool homebrew_proto_active(void *homebrewptr)
{
    dmr_homebrew_t *homebrew = (dmr_homebrew_t *)homebrewptr;
    if (homebrew == NULL)
        return false;

    return homebrew->thread != NULL && homebrew->run;
}

static void homebrew_proto_rx(void *homebrewptr, dmr_packet_t *packet)
{
    dmr_homebrew_t *homebrew = (dmr_homebrew_t *)homebrewptr;
    if (homebrew == NULL || packet == NULL)
        return;

    dmr_proto_rx(&homebrew->proto, homebrew, packet);
}

dmr_homebrew_t *dmr_homebrew_new(struct in_addr bind, int port, struct in_addr peer)
{
    int optval = 1;
    dmr_homebrew_t *homebrew = malloc(sizeof(dmr_homebrew_t));
    if (homebrew == NULL)
        return NULL;

    memset(homebrew, 0, sizeof(dmr_homebrew_t));
    homebrew->config = dmr_homebrew_config_new();
    if (homebrew->config == NULL)
        return NULL;

    // Setup protocol
    homebrew->proto.name = dmr_homebrew_proto_name;
    homebrew->proto.type = DMR_PROTO_HOMEBREW;
    homebrew->proto.init = homebrew_proto_init;
    homebrew->proto.start = homebrew_proto_start;
    homebrew->proto.stop = homebrew_proto_stop;
    homebrew->proto.active = homebrew_proto_active;
    homebrew->proto.rx = homebrew_proto_rx;

    // Setup file descriptor for UDP socket
    homebrew->fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (homebrew->fd < 0) {
        fprintf(stderr, "homebrew: error creating socket: %s\n",
            strerror(errno));
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

    fprintf(stderr, "homebrew: remote %s\n", inet_ntoa(peer));
    homebrew->remote.sin_family = AF_INET;
    homebrew->remote.sin_addr.s_addr = peer.s_addr;
    homebrew->remote.sin_port = htons(port);
    memset(homebrew->remote.sin_zero, 0, sizeof(homebrew->remote.sin_zero));

    return homebrew;
}

bool dmr_homebrew_auth(dmr_homebrew_t *homebrew, const char *secret)
{
    uint8_t buf[328], digest[SHA256_DIGEST_LENGTH];
    int i, j;
    uint16_t len;
    sha256_t sha256ctx;
    //memset(&buf, 0, 64);

    fprintf(stderr, "homebrew: connecting to repeater at %s:%d as %02x%02x%02x%02x\n",
        inet_ntoa(homebrew->remote.sin_addr),
        ntohs(homebrew->remote.sin_port),
        homebrew->config->repeater_id[0],
        homebrew->config->repeater_id[1],
        homebrew->config->repeater_id[2],
        homebrew->config->repeater_id[3]);
    while (homebrew->auth != DMR_HOMEBREW_AUTH_FAIL) {
        switch (homebrew->auth) {
        case DMR_HOMEBREW_AUTH_NONE:
            memcpy(buf, dmr_homebrew_repeater_login, 4);
            for (i = 0, j = 0; i < 4; i++, j += 2) {
                buf[j+4] = hex[(homebrew->config->repeater_id[i] >> 4)];
                buf[j+5] = hex[(homebrew->config->repeater_id[i] & 0xf)];
            }
            fprintf(stderr, "homebrew: sending repeater id\n");
            if (!dmr_homebrew_sendraw(homebrew, buf, 12))
                return false;

            while (true) {
                if (!dmr_homebrew_recvraw(homebrew, &len))
                    return false;

                if (len == 14 && !memcmp(&homebrew->buffer[0], dmr_homebrew_master_nak, 6)) {
                    fprintf(stderr, "homebrew: our DMR id was refused by the master\n");
                    homebrew->auth = DMR_HOMEBREW_AUTH_FAIL;
                    break;
                }
                if (len == 22 && !memcmp(&homebrew->buffer[0], dmr_homebrew_master_ack, 6)) {
                    memcpy(&homebrew->random, &homebrew->buffer[14], 8);
                    fprintf(stderr, "homebrew: master accepted our repeater id, random: %.*s\n",
                        8, homebrew->random);
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

            fprintf(stderr, "homebrew: sending nonce\n");
            if (!dmr_homebrew_sendraw(homebrew, buf, 76))
                return false;

            while (true) {
                if (!dmr_homebrew_recvraw(homebrew, &len))
                    return false;

                if (len == 14 && !memcmp(&homebrew->buffer[0], dmr_homebrew_master_nak, 6)) {
                    fprintf(stderr, "homebrew: our secret was refused by the master\n");
                    homebrew->auth = DMR_HOMEBREW_AUTH_FAIL;
                    break;
                }
                if (len == 14 && !memcmp(&homebrew->buffer[0], dmr_homebrew_master_ack, 6)) {
                    fprintf(stderr, "homebrew: master accepted nonce, logged in\n");
                    homebrew->auth = DMR_HOMEBREW_AUTH_DONE;
                    break;
                }

            }
            break;

        case DMR_HOMEBREW_AUTH_FAIL:
            fprintf(stderr, "homebrew: login failed\n");
            return false;

        case DMR_HOMEBREW_AUTH_DONE:
            fprintf(stderr, "homebrew: logged in, sending our configuration\n");
            memcpy(&buf, &dmr_homebrew_repeater_config, 4);
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
            if (!dmr_homebrew_sendraw(homebrew, buf, pos))
                return false;

            return true;
        }
    }

    return true;
}

void dmr_homebrew_close(dmr_homebrew_t *homebrew)
{
    uint8_t buf[12];
    int i, j;

    if (homebrew == NULL)
        return;

    if (homebrew->fd < 0)
        return;

    if (homebrew->run) {
        homebrew->run = false;
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

    if (homebrew->run) {
        dmr_homebrew_close(homebrew);
#ifdef DMR_PLATFORM_WINDOWS
        Sleep(1000);
#else
        sleep(1);
#endif

        close(homebrew->fd);
    }

    free(homebrew->config);
    free(homebrew);
}

void dmr_homebrew_loop(dmr_homebrew_t *homebrew)
{
    uint8_t buf[53];
    uint16_t len, pos;
    dmr_packet_t packet, *p;

    if (homebrew == NULL)
        return;

    homebrew->run = true;
    while (homebrew->run) {
        if (!dmr_homebrew_recvraw(homebrew, &len))
            return;

        if (len == 15 && !memcmp(&homebrew->buffer[0], dmr_homebrew_master_ping, 7)) {
            fprintf(stderr, "homebrew: ping? pong!\n");
            memcpy(buf, homebrew->buffer, 15);
            memcpy(buf, dmr_homebrew_repeater_pong, 7);
            if (!dmr_homebrew_sendraw(homebrew, buf, 15))
                return;
        } else if (len > 4 && !memcmp(&homebrew->buffer, dmr_homebrew_data_signature, 4)) {
            p = &packet;
            memset(p, 0, sizeof(dmr_packet_t));
            pos = 4 + 1;
            packet.src_id |= (homebrew->buffer[pos++] << 16);
            packet.src_id |= (homebrew->buffer[pos++] << 8);
            packet.src_id |= (homebrew->buffer[pos++] << 0);
            packet.dst_id |= (homebrew->buffer[pos++] << 16);
            packet.dst_id |= (homebrew->buffer[pos++] << 8);
            packet.dst_id |= (homebrew->buffer[pos++] << 0);
            packet.repeater_id |= (homebrew->buffer[pos++] << 0);
            packet.repeater_id |= (homebrew->buffer[pos++] << 8);
            packet.repeater_id |= (homebrew->buffer[pos++] << 16);
            packet.repeater_id |= (homebrew->buffer[pos++] << 24);
            pos += 1 + 4;
            memcpy(&packet.payload.bytes[0], &homebrew->buffer[pos], 33);
            homebrew->proto.rx(homebrew, p);
        }
    }
}

bool dmr_homebrew_send(dmr_homebrew_t *homebrew, dmr_ts_t ts, dmr_packet_t *packet)
{
    uint8_t buf[53], pos = 0;

    if (homebrew == NULL || packet == NULL || ts > DMR_TS2)
        return false;

    if (packet->repeater_id == 0)
        packet->repeater_id = homebrew->id;

    if (homebrew->tx[ts].src_id     != packet->src_id    ||
        homebrew->tx[ts].dst_id     != packet->dst_id    ||
        homebrew->tx[ts].call_type  != packet->call_type ||
        homebrew->tx[ts].slot_type  != packet->slot_type) {
        homebrew->tx[ts].src_id      = packet->src_id;
        homebrew->tx[ts].dst_id      = packet->dst_id;
        homebrew->tx[ts].call_type   = packet->call_type;
        homebrew->tx[ts].slot_type   = packet->slot_type;
        homebrew->tx[ts].seq         = 0;
        homebrew->tx[ts].stream_id  += 1;
    }

    memcpy(&buf[0], dmr_homebrew_data_signature, 4);
    pos += 4;
    buf[pos++] = homebrew->tx[ts].seq++;
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
    buf[pos] = (ts & 0x01) | (packet->call_type << 1);
    switch (packet->slot_type) {
    case DMR_SLOT_TYPE_VOICE_BURST_A:
        buf[pos] |= (0x00 << 2);
        buf[pos] |= (packet->slot_type - DMR_SLOT_TYPE_VOICE_BURST_A) << 4;
        break;
    case DMR_SLOT_TYPE_VOICE_BURST_B:
    case DMR_SLOT_TYPE_VOICE_BURST_C:
    case DMR_SLOT_TYPE_VOICE_BURST_D:
    case DMR_SLOT_TYPE_VOICE_BURST_E:
    case DMR_SLOT_TYPE_VOICE_BURST_F:
        buf[pos] |= (0x01 << 2);
        buf[pos] |= (packet->slot_type - DMR_SLOT_TYPE_VOICE_BURST_A) << 4;
        break;
    default:
        buf[pos] |= (0x02 << 2);
        buf[pos] |= (packet->slot_type & 0x0f);
        break;
    }
    pos++;
    buf[pos++] = (homebrew->tx[ts].stream_id >> 24);
    buf[pos++] = (homebrew->tx[ts].stream_id >> 16);
    buf[pos++] = (homebrew->tx[ts].stream_id >> 8);
    buf[pos++] = (homebrew->tx[ts].stream_id >> 0);
    memcpy(&buf[pos], packet->payload.bytes, DMR_PAYLOAD_BYTES);

    return dmr_homebrew_sendraw(homebrew, buf, DMR_PAYLOAD_BYTES + 20);
}

bool dmr_homebrew_sendraw(dmr_homebrew_t *homebrew, const uint8_t *buf, uint16_t len)
{
    if (homebrew == NULL)
        return false;

    fprintf(stderr, "homebrew: send(%s, %d): ", inet_ntoa(homebrew->remote.sin_addr), len);
    dump_hex((void *)buf, len);
    if (sendto(homebrew->fd, (const char *)buf, len, 0, (struct sockaddr *)&homebrew->remote, sizeof(homebrew->remote)) != len) {
        fprintf(stderr, "homebrew: send to %s:%d failed: %s\n",
            inet_ntoa(homebrew->remote.sin_addr),
            ntohs(homebrew->remote.sin_port),
            strerror(errno));
        return false;
    }
    return true;
}

bool dmr_homebrew_recvraw(dmr_homebrew_t *homebrew, uint16_t *len)
{
    struct sockaddr_in peer;
    int ilen;
#ifdef DMR_PLATFORM_WINDOWS
    int peerlen;
#else
    socklen_t peerlen;
#endif
    uint16_t olen;
    if ((ilen = recvfrom(homebrew->fd, (char *)&homebrew->buffer, 64, 0, (struct sockaddr *)&peer, &peerlen)) < 0) {
        *len = 0;
        fprintf(stderr, "homebrew: recv from %s:%d failed: %s\n",
            inet_ntoa(homebrew->remote.sin_addr),
            ntohs(homebrew->remote.sin_port),
            strerror(errno));
        return false;
    }

    olen = ilen;
    *len = olen;
    dump_hex(homebrew->buffer, *len);
    return true;
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
    if (dmr_homebrew_frame_type(bytes, len) != DMR_HOMEBREW_DMR_DATA_FRAME)
        return NULL;

    dmr_homebrew_packet_t *packet = malloc(sizeof(dmr_homebrew_packet_t));
    if (bytes == NULL)
        return NULL;

    memset(packet, 0, sizeof(dmr_homebrew_packet_t));
    memcpy(packet->signature, bytes, 4);
    packet->sequence = bytes[4];
    packet->dmr_packet.src_id = (bytes[5] << 16) | (bytes[6] << 8) | bytes[7];
    packet->dmr_packet.dst_id = (bytes[8] << 16) | (bytes[9] << 8) | bytes[10];
    packet->dmr_packet.repeater_id = bytes[11] | (bytes[12] << 8) | (bytes[13] << 16) | (bytes[14] << 24);
    packet->dmr_packet.ts = (bytes[15]) & 0x01;
    packet->call_type = (bytes[15] >> 1) & 0x01;
    packet->dmr_packet.call_type = packet->call_type;
    packet->frame_type = (bytes[15] >> 2) & 0x03;
    packet->data_type = (bytes[15] >> 4) & 0x0f;
    switch (packet->frame_type) {
    case DMR_HOMEBREW_DATA_TYPE_VOICE:
    case DMR_HOMEBREW_DATA_TYPE_VOICE_SYNC:
        packet->dmr_packet.slot_type = packet->data_type + DMR_SLOT_TYPE_VOICE_BURST_A;
        break;
    default:
        packet->dmr_packet.slot_type = packet->data_type;
        break;
    }

    switch (packet->frame_type) {
    case DMR_HOMEBREW_DATA_TYPE_VOICE:
    case DMR_HOMEBREW_DATA_TYPE_VOICE_SYNC:
        packet->dmr_packet.slot_type = DMR_SLOT_TYPE_VOICE_BURST_A + packet->data_type;
		break;
	case DMR_HOMEBREW_DATA_TYPE_DATA_SYNC: // data sync
		packet->dmr_packet.slot_type = (dmr_slot_type_t) packet->data_type;
		break;
    default:
        packet->dmr_packet.slot_type = DMR_SLOT_TYPE_UNKNOWN;
        break;
    }

    memcpy(&packet->dmr_packet.payload.bytes, &bytes[20], DMR_PAYLOAD_BYTES);
    bytes_to_bits(&packet->dmr_packet.payload.bytes[0], DMR_PAYLOAD_BYTES,
                  &packet->dmr_packet.payload.bits[0], DMR_PAYLOAD_BITS);

    return packet;
}
