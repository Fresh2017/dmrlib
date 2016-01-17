#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <ctype.h>

#include "dmr/platform.h"
#include "dmr/time.h"

#if defined(DMR_PLATFORM_WINDOWS)
#include <windows.h>
#include <assert.h>
#else
#include <fcntl.h>
#include <termios.h>
#endif

#include "dmr/error.h"
#include "dmr/log.h"
#include "dmr/payload/sync.h"
#include "dmr/proto.h"
#include "dmr/proto/mmdvm.h"
#include "dmr/thread.h"
#include "dmr/serial.h"
#include "dmr/malloc.h"

static const char *dmr_mmdvm_proto_name = "mmdvm";

static int mmdvm_proto_init(void *modemptr)
{
    dmr_log_trace("mmdvm: init");
    dmr_mmdvm_t *modem = (dmr_mmdvm_t *)modemptr;
    if (modem == NULL)
        return dmr_error(DMR_EINVAL);

    if (modem->flag & DMR_MMDVM_FLAG_SYNC) {
        if (dmr_mmdvm_sync(modem) != 0) {
            dmr_log_error("mmdvm: sync failed");
            return dmr_error(DMR_UNSUPPORTED);
        }
    }
    return 0;
}

static bool mmdvm_proto_active(void *modemptr)
{
    dmr_log_mutex("mmdvm: active");
    dmr_mmdvm_t *modem = (dmr_mmdvm_t *)modemptr;
    if (modem == NULL)
        return false;

    dmr_log_mutex("mmdvm: mutex lock");
    dmr_mutex_lock(modem->proto.mutex);
    bool active = modem->proto.thread != NULL && modem->proto.is_active;
    dmr_log_mutex("mmdvm: mutex unlock");
    dmr_mutex_unlock(modem->proto.mutex);
    dmr_log_mutex("mmdvm: active = %s", DMR_LOG_BOOL(active));
    return active;
}

static int mmdvm_proto_start_thread(void *modemptr)
{
    dmr_log_trace("mmdvm: start thread");
    dmr_mmdvm_t *modem = (dmr_mmdvm_t *)modemptr;
    if (modem == NULL) {
        dmr_log_error("mmdvm: start thread called without modem?!\n");
        return dmr_thread_error;
    }
    dmr_thread_name_set("mmdvm proto");
    while (mmdvm_proto_active(modem)) {
        switch (dmr_mmdvm_poll(modem)) {
        case dmr_mmdvm_error:
            dmr_log_critical("mmdvm: stop on error: %s", strerror(modem->error));
            dmr_mutex_lock(modem->proto.mutex);
            modem->proto.is_active = false;
            dmr_mutex_unlock(modem->proto.mutex);
            break;
        case dmr_mmdvm_timeout:
        default:
            dmr_msleep(25);
            break;
        }
    }
    dmr_thread_exit(dmr_thread_success);
    return dmr_thread_success;
}

static int mmdvm_proto_start(void *modemptr)
{
    dmr_log_trace("mmdvm: start");
    dmr_mmdvm_t *modem = (dmr_mmdvm_t *)modemptr;
    if (modem == NULL)
        return dmr_error(DMR_EINVAL);

    if (modem->proto.thread != NULL) {
        dmr_log_error("mmdvm: can't start, already active");
        return dmr_error(DMR_EINVAL);
    }

    modem->proto.thread = malloc(sizeof(dmr_thread_t));
    if (modem->proto.thread == NULL) {
        dmr_log_critical("mmdvm: can't start, out of memory");
        return dmr_error(DMR_ENOMEM);
    }

    dmr_log_mutex("mmdvm: mutex lock");
    dmr_mutex_lock(modem->proto.mutex);
    modem->proto.is_active = true;
    dmr_log_mutex("mmdvm: mutex unlock");
    dmr_mutex_unlock(modem->proto.mutex);
    if (dmr_thread_create(modem->proto.thread, mmdvm_proto_start_thread, modemptr) != dmr_thread_success) {
        dmr_log_error("mmdvm: can't creat thread");
        return dmr_error(DMR_EINVAL);
    }

    return 0;
}

static int mmdvm_proto_stop(void *modemptr)
{
    dmr_log_trace("mmdvm: stop");
    dmr_mmdvm_t *modem = (dmr_mmdvm_t *)modemptr;
    if (modem == NULL)
        return dmr_error(DMR_EINVAL);

    if (modem->proto.thread == NULL) {
        dmr_log_error("mmdvm: not active\n");
        return 0;
    }

    dmr_mutex_lock(modem->proto.mutex);
    modem->proto.is_active = false;
    dmr_mutex_unlock(modem->proto.mutex);
    if (dmr_thread_join(*modem->proto.thread, NULL) != dmr_thread_success) {
        dmr_log_error("mmdvm: can't join thread");
        return dmr_error(DMR_EINVAL);
    }

    free(modem->proto.thread);
    modem->proto.thread = NULL;

    dmr_mmdvm_close(modem);
    return 0;
}

static void mmdvm_proto_tx(void *modemptr, dmr_packet_t *packet)
{
    dmr_log_trace("mmdvm: tx");
    dmr_mmdvm_t *modem = (dmr_mmdvm_t *)modemptr;
    if (modem == NULL || packet == NULL)
        return;

    uint8_t buf[37];
    switch (packet->data_type) {
    case DMR_DATA_TYPE_VOICE:
    case DMR_DATA_TYPE_VOICE_SYNC:
        dmr_log_trace("mmdvm: sending voice frames to modem");
        if (modem->last_mode != DMR_MMDVM_MODE_DMR) {
            dmr_log_trace("mmdvm: putting modem in DMR mode");
            buf[0] = DMR_MMDVM_FRAME_START;
            buf[1] = 3;
            buf[2] = 0x1c; /* DMR transmit start */
            if (dmr_serial_write(modem->fd, buf, 3) != 3) {
                dmr_log_error("mmdvm: failed to send DMR start to modem");
                return;
            }
            modem->last_mode = DMR_MMDVM_MODE_DMR;
        }
        buf[0] = DMR_MMDVM_FRAME_START;
        buf[1] = 37;
        buf[2] = 0x1a;
        buf[3] = (packet->ts & 0x01) << 7;
        memcpy(&buf[4], packet->payload, 33);
        if (dmr_serial_write(modem->fd, buf, sizeof(buf)) != sizeof(buf)) {
            dmr_log_error("mmdvm: failed to send %lu bytes of DMR data to modem",
                sizeof(buf));
        }
        break;

    case DMR_DATA_TYPE_TERMINATOR_WITH_LC:
        if (modem->last_mode != DMR_MMDVM_MODE_DMR) {
            dmr_log_debug("mmdvm: received voice terminator, but not in DMR mode (ignored)");
            return;
        }
        dmr_log_trace("mmdvm: sending voice terminator to modem");
        buf[0] = DMR_MMDVM_FRAME_START;
        buf[1] = 4;
        buf[2] = DMR_MMDVM_DMR_LOST1;
        buf[3] = (packet->ts & 0x01) << 7;
        if (dmr_serial_write(modem->fd, buf, 4) != 4) {
            dmr_log_error("mmdvm: failed to send DMR EOT");
        }
        modem->last_mode = DMR_MMDVM_MODE_INVALID;
        break;

    default: // ignored
        break;
    }
}

static char *dmr_mmdvm_command_name(uint8_t command)
{
    switch (command) {
    case DMR_MMDVM_GET_VERSION:
        return "get version";
    case DMR_MMDVM_GET_STATUS:
        return "get status";
    case DMR_MMDVM_SET_CONFIG:
        return "set config";
    case DMR_MMDVM_SET_MODE:
        return "set mode";

    case DMR_MMDVM_DSTAR_HEADER:
        return "dstar header";
    case DMR_MMDVM_DSTAR_DATA:
        return "dstar data";
    case DMR_MMDVM_DSTAR_LOST:
        return "dstar lost";
    case DMR_MMDVM_DSTAR_EOT:
        return "dstar eot";

    case DMR_MMDVM_DMR_DATA1:
        return "dmr ts1 data";
    case DMR_MMDVM_DMR_LOST1:
        return "dmr ts1 lost";
    case DMR_MMDVM_DMR_DATA2:
        return "dmr ts2 data";
    case DMR_MMDVM_DMR_LOST2:
        return "dmr ts2 lost";
    case DMR_MMDVM_DMR_SHORTLC:
        return "dmr short lc";
    case DMR_MMDVM_DMR_START:
        return "dmr start";

    case DMR_MMDVM_YSF_DATA:
        return "ysf data";
    case DMR_MMDVM_YSF_LOST:
        return "ysf lost";

    case DMR_MMDVM_ACK:
        return "ack";
    case DMR_MMDVM_NAK:
        return "nak";

    case DMR_MMDVM_DUMP:
        return "dump";
    case DMR_MMDVM_DEBUG1:
        return "debug 1";
    case DMR_MMDVM_DEBUG2:
        return "debug 2";
    case DMR_MMDVM_DEBUG3:
        return "debug 3";
    case DMR_MMDVM_DEBUG4:
        return "debug 4";
    case DMR_MMDVM_DEBUG5:
        return "debug 5";
    case DMR_MMDVM_SAMPLES:
        return "samples";

    default:
        return "unknown";
    }
}

#ifdef DMR_MMDVM_DUMP_FILE
FILE *dump;
#endif

dmr_mmdvm_t *dmr_mmdvm_open(char *port, long baud, uint16_t flag, size_t buffer_sizes)
{
    dmr_mmdvm_t *modem = dmr_malloc_zero(dmr_mmdvm_t);
    if (modem == NULL)
        return NULL;

    memset(modem, 0, sizeof(dmr_mmdvm_t));
    dmr_ts_t ts;
    for (ts = DMR_TS1; ts < DMR_TS_INVALID; ts++) {
        modem->dmr_ts[ts].last_data_type = DMR_DATA_TYPE_INVALID;
    }

#ifdef DMR_MMDVM_DUMP_FILE
    dump = fopen("mmdvm.dump", "w+");
#endif

    // Setup MMDVM protocol
    modem->proto.name = dmr_mmdvm_proto_name;
    modem->proto.init = mmdvm_proto_init;
    modem->proto.start = mmdvm_proto_start;
    modem->proto.stop = mmdvm_proto_stop;
    modem->proto.active = mmdvm_proto_active;
    modem->proto.tx = mmdvm_proto_tx;
    if (dmr_proto_mutex_init(&modem->proto) != 0) {
        dmr_log_error("modem: failed to init mutex");
        free(modem);
        return NULL;
    }

    // Setup MMDVM modem
    modem->version = 0;
    modem->firmware = NULL;
    modem->baud = baud;
    modem->flag = flag;
    modem->last_mode = DMR_MMDVM_MODE_INVALID;
#ifdef DMR_PLATFORM_WINDOWS
    char portbuf[64];
    memset(portbuf, 0, 64);
    snprintf(portbuf, 64, "\\\\.\\%s", port);
    modem->port = strdup(portbuf);
#else
    modem->port = strdup(port);
#endif
    if (modem->port == NULL) {
        dmr_error(DMR_ENOMEM);
        dmr_mmdvm_free(modem);
        return NULL;
    }

    if ((modem->fd = dmr_serial_open(modem->port, baud, true)) < 0) {
        modem->error = errno;
        dmr_log_error("mmdvm: open %s failed: %s", modem->port, dmr_serial_error_message());
        dmr_mmdvm_free(modem);
        return NULL;
    }

    return modem;
}

static void dmr_mmdvm_poll_timeouts(dmr_mmdvm_t *modem)
{
    dmr_ts_t ts;
    for (ts = 0; ts < DMR_TS2; ts++) {
        if (modem->dmr_ts[ts].last_data_type == DMR_DATA_TYPE_VOICE ||
            modem->dmr_ts[ts].last_data_type == DMR_DATA_TYPE_VOICE_SYNC) {
            uint32_t delta = dmr_time_ms_since(modem->dmr_ts[ts].last_packet_received);
            dmr_log_trace("mmdvm: last %s was %ums ago",
                dmr_data_type_name(modem->dmr_ts[ts].last_data_type), delta);

            if (delta > 120) {
                dmr_log_debug("mmdvm: voice stream on %s closed after %ums", dmr_ts_name(ts), delta);

                // Append terminator with LC header
                /*
                dmr_packet_t *packet = dmr_malloc_zero(dmr_packet_t);
                if (packet == NULL) {
                    dmr_error(DMR_ENOMEM);
                    return;
                }
                packet->data_type = DMR_DATA_TYPE_TERMINATOR_WITH_LC;
                packet->ts = ts;
                packet->meta.sequence = modem->dmr_ts[ts].last_sequence++ % 0xff;
                dmr_proto_rx_cb_run(&modem->proto, packet);
                */

                // Ignore in next iteration
                dmr_log_debug("mmdvm: change in data type %s -> %s on %s",
                    dmr_data_type_name(modem->dmr_ts[ts].last_data_type),
                    dmr_data_type_name(DMR_DATA_TYPE_INVALID),
                    dmr_ts_name(ts));
                modem->dmr_ts[ts].last_data_type = DMR_DATA_TYPE_INVALID;

                // Cleanup
                //dmr_free(packet);
            }
        }
    }
}

int dmr_mmdvm_poll(dmr_mmdvm_t *modem)
{
    if (modem == NULL || modem->fd == 0)
        return dmr_error(DMR_EINVAL);

    uint8_t len, length;
    uint16_t val[4];
    struct timeval timeout = { 1, 0 };
    dmr_mmdvm_response_t res = dmr_mmdvm_get_response(modem, &len, &timeout, 1);

    dmr_ts_t ts = DMR_TS_INVALID;
    switch (res) {
    case dmr_mmdvm_timeout:
        dmr_mmdvm_poll_timeouts(modem);
        return 0; // Nothing to do here

    case dmr_mmdvm_error:
        if (len == 0) {
            dmr_mmdvm_poll_timeouts(modem);
            return 0; // Nothing to do here
        }

        dmr_log_debug("mmdvm: error reading response");
        return 1;

    case dmr_mmdvm_ok:
        dmr_mmdvm_poll_timeouts(modem);

        length = modem->buffer[1];
        switch (modem->buffer[2]) {
        case DMR_MMDVM_DMR_DATA1:
        case DMR_MMDVM_DMR_DATA2:
            dmr_log_debug("mmdvm: DMR data on TS%d, type 0x%02x (%d)",
                modem->buffer[2] - DMR_MMDVM_DMR_DATA1,
                modem->buffer[3],
                modem->buffer[3]);
            dmr_dump_hex(modem->buffer, len);

            dmr_packet_t *packet = dmr_packet_decode(&modem->buffer[4], len - 4);
            if (packet == NULL) {
                dmr_log_error("mmdvm: unable to decode DMR packet");
                return -1;
            }

            // This can actually be ts 0-2 (so TS3!), because that's how mmdvm
            // works in hot spot mode.
            packet->data_type = modem->buffer[3];
            packet->ts = (modem->buffer[3] & DMR_MMDVM_DMR_TS) > 0 ? DMR_TS2 : DMR_TS1;
            if ((modem->buffer[3] & DMR_MMDVM_DMR_VOICE_SYNC) > 0) {
                dmr_log_debug("mmdvm: voice sync");
                packet->data_type = DMR_DATA_TYPE_VOICE_SYNC;
            } else if ((modem->buffer[3] & DMR_MMDVM_DMR_DATA_SYNC) > 0) {
                dmr_log_debug("mmdvm: data sync (not implemented)");
                return 0;
            } else {
                dmr_log_debug("mmdvm: voice");
                packet->data_type = DMR_DATA_TYPE_VOICE;
            }

            bool reset = false;
            uint32_t delta = dmr_time_ms_since(modem->dmr_ts[ts].last_packet_received);
            if ((packet->data_type                 == DMR_DATA_TYPE_VOICE ||
                  packet->data_type                == DMR_DATA_TYPE_VOICE_SYNC) &&
                 (modem->dmr_ts[ts].last_data_type != DMR_DATA_TYPE_VOICE &&
                  modem->dmr_ts[ts].last_data_type != DMR_DATA_TYPE_VOICE_SYNC)) {
                dmr_log_debug("mmdvm: change in data type %s -> %s on %s, starting new stream",
                    dmr_data_type_name(modem->dmr_ts[ts].last_data_type),
                    dmr_data_type_name(packet->data_type),
                    dmr_ts_name(ts));
                reset = true;
            } else if (delta > 120) {
                dmr_log_debug("mmdvm: last packet received %ums ago, starting new stream",
                    delta);
                reset = true;
            }

            if (reset) {
                modem->dmr_ts[ts].last_sequence = 0;
                modem->dmr_ts[ts].last_voice_frame = 0;
            }

            packet->flco = DMR_FLCO_GROUP;
            packet->meta.sequence = modem->dmr_ts[ts].last_sequence++ % 0xff;
            switch (packet->data_type) {
            case DMR_DATA_TYPE_VOICE:
            case DMR_DATA_TYPE_VOICE_SYNC:
            case DMR_DATA_TYPE_SYNC_VOICE:
                packet->meta.voice_frame = modem->dmr_ts[ts].last_voice_frame++ % 6;

                // Number it as a valid voice burst frame
                if (packet->meta.voice_frame == 0) { /* burst A */
                    packet->data_type = DMR_DATA_TYPE_VOICE_SYNC;
                    dmr_sync_pattern_encode(DMR_SYNC_PATTERN_BS_SOURCED_DATA, packet);
                } else { /* burst B-F */
                    packet->data_type = DMR_DATA_TYPE_VOICE;
                    // We let the other protocols take care of inserting the EMB LC,
                    // because we have no src_id/dst_id anyway.
                }
                break;

            default:
                break;
            }

            // For the start of a new voice call, we prepend an (empty) LC header
            if (packet->data_type == DMR_DATA_TYPE_VOICE_SYNC && reset) {
                dmr_packet_t *header = dmr_malloc_zero(dmr_packet_t);
                if (header == NULL) {
                    return dmr_error(DMR_ENOMEM);
                }

                header->data_type = DMR_DATA_TYPE_VOICE_LC;
                header->ts = packet->ts;
                header->flco = packet->flco;
                header->meta.sequence = packet->meta.sequence;
                packet->meta.sequence++;
                dmr_sync_pattern_encode(DMR_SYNC_PATTERN_BS_SOURCED_DATA, header);
                dmr_log_trace("mmdvm: rx %s (prepended)", dmr_data_type_name(header->data_type));
                dmr_proto_rx_cb_run(&modem->proto, header);

                dmr_free(header);
            }

            dmr_log_trace("mmdvm: rx %s", dmr_data_type_name(packet->data_type));
            dmr_proto_rx_cb_run(&modem->proto, packet);

            // Book keeping
            gettimeofday(&modem->dmr_ts[ts].last_packet_received, NULL); 
            modem->dmr_ts[ts].last_data_type = packet->data_type;
            modem->last_dmr_ts = ts;

            // Cleanup
            dmr_free(packet);

            break;

        case DMR_MMDVM_DMR_LOST1:
        case DMR_MMDVM_DMR_LOST2:
            ts = (modem->buffer[3] & DMR_MMDVM_DMR_TS) > 0 ? DMR_TS2 : DMR_TS1;
            dmr_log_debug("mmdvm: DMR lost %u", dmr_ts_name(ts));
            
            // Expire our last packet received and run the timeout handler.
            modem->dmr_ts[ts].last_packet_received.tv_sec = 0;
            dmr_mmdvm_poll_timeouts(modem);
            break;

        case DMR_MMDVM_GET_STATUS:
            dmr_log_debug("mmdvm: get status response");
            break;

        case DMR_MMDVM_GET_VERSION:
            dmr_log_debug("mmdvm: get version response");
            break;

		case DMR_MMDVM_ACK:
            dmr_log_debug("mmdvm: ack");
			break;

        case DMR_MMDVM_NAK:
            dmr_log_info("mmdvm: received NAK for command %s (0x%02x), reason: %u",
                dmr_mmdvm_command_name(modem->buffer[3]),
                modem->buffer[3],
                modem->buffer[4]);
            break;

        case DMR_MMDVM_DEBUG1:
        case DMR_MMDVM_DEBUG2:
        case DMR_MMDVM_DEBUG3:
        case DMR_MMDVM_DEBUG4:
        case DMR_MMDVM_DEBUG5:
            switch (modem->buffer[2]) {
            case DMR_MMDVM_DEBUG1:
                dmr_log_error("mmdvm: debug1 %.*s\n",
                    length - 3, &modem->buffer[3]);
                break;
            case DMR_MMDVM_DEBUG2:
                val[0] = modem->buffer[length - 2] << 8 | modem->buffer[length - 1];
                dmr_log_error("mmdvm: debug2 %.*s %d\n",
                    length - 5, &modem->buffer[3], val[0]);
                break;
            case DMR_MMDVM_DEBUG3:
                val[0] = modem->buffer[length - 4] << 8 | modem->buffer[length - 3];
                val[1] = modem->buffer[length - 2] << 8 | modem->buffer[length - 1];
                dmr_log_error("mmdvm: debug3 %.*s %d %d\n",
                    length - 7, &modem->buffer[3], val[0], val[1]);
                break;
            case DMR_MMDVM_DEBUG4:
                val[0] = modem->buffer[length - 6] << 8 | modem->buffer[length - 5];
                val[1] = modem->buffer[length - 4] << 8 | modem->buffer[length - 3];
                val[2] = modem->buffer[length - 2] << 8 | modem->buffer[length - 1];
                dmr_log_error("mmdvm: debug4 %.*s %d %d %d\n",
                    length - 9, &modem->buffer[3], val[0], val[1], val[2]);
                break;
            case DMR_MMDVM_DEBUG5:
                val[0] = modem->buffer[length - 8] << 8 | modem->buffer[length - 7];
                val[1] = modem->buffer[length - 6] << 8 | modem->buffer[length - 5];
                val[2] = modem->buffer[length - 4] << 8 | modem->buffer[length - 3];
                val[3] = modem->buffer[length - 2] << 8 | modem->buffer[length - 1];
                dmr_log_error("mmdvm: debug5 %.*s %d %d %d %d\n",
                    length - 11, &modem->buffer[3], val[0], val[1], val[2], val[3]);
                break;
            }
            break;

        case DMR_MMDVM_SAMPLES:
            dmr_log_debug("mmdvm: samples");
            break; // FIXME(maze): not implemented

        default:
            dmr_log_warn("mmdvm: modem sent unhandled response %d (%s), length %d\n",
                modem->buffer[2], dmr_mmdvm_command_name(modem->buffer[2]),
                modem->buffer[1]);
            dmr_dump_hex(&modem->buffer[3], length - 2);
            break;
        }

        gettimeofday(&modem->last_packet_received, NULL);
    }
    return 0;
}

int dmr_mmdvm_sync(dmr_mmdvm_t *modem)
{
    uint8_t i;

    if (modem == NULL)
        return dmr_error(DMR_EINVAL);

    dmr_log_debug("mmdvm: sync modem on %s", modem->port);
    for (i = 0; i < 6; i++) {
        uint8_t buf[3] = {DMR_MMDVM_FRAME_START, 3, DMR_MMDVM_GET_VERSION};
        if (dmr_serial_write(modem->fd, &buf[0], 3) != 3) {
            modem->error = errno;
            return dmr_error(DMR_EWRITE);
        }
        dmr_msleep(DMR_MMDVM_DELAY_MS);

        uint8_t len;
        struct timeval timeout = { 1, 0 };
        dmr_mmdvm_response_t res = dmr_mmdvm_get_response(modem, &len, &timeout, 16);
        if (res == dmr_mmdvm_timeout)
            goto mmdvm_sync_retry;
        if (len < 2) {
            dmr_log_error("mmdvm: response too short: received %d bytes", len);
            goto mmdvm_sync_retry;
        }
        dmr_log_debug("mmdvm: %d bytes of response", len);
        dmr_dump_hex(modem->buffer, len);
        if (res == dmr_mmdvm_ok && modem->buffer[2] == DMR_MMDVM_GET_VERSION) {
            dmr_log_info("mmdvm: protocol %d, %.*s",
                modem->buffer[3], len - 4, &modem->buffer[4]);

            modem->version = modem->buffer[3];
            modem->firmware = malloc(len - 3);
            memset(modem->firmware, 0, len - 3);
            memcpy(modem->firmware, &modem->buffer[4], len - 4);
            return 0;
        }

mmdvm_sync_retry:
        dmr_sleep(1);
    }

    dmr_log_error("mmdvm: sync error: unable to read firmware version");
    return dmr_error(DMR_EINVAL);
}

dmr_mmdvm_response_t dmr_mmdvm_get_response(dmr_mmdvm_t *modem, uint8_t *length, struct timeval *timeout, int retries)
{
    int ret = 0;
    uint8_t offset = 0, retry = 0;
    uint8_t *p = modem->buffer;

    if (modem == NULL)
        return dmr_mmdvm_error;

    // Clear buffer
    memset(modem->buffer, 0, sizeof(modem->buffer));
    *length = 0;

    while (true) {
        if (offset == 0) {
            // Nothing received yet, scan for a DMR_MMDVM_FRAME_START
            //dmr_log_trace("mmdvm: looking for DMR_MMDVM_FRAME_START %d/%d", retry, retries);
            if (retry >= retries && retries > 1) {
                dmr_log_debug("mmdvm: maximum retries %d reached", retry);
                return dmr_mmdvm_error;
            }
            ret = dmr_serial_read(modem->fd, p, 1, timeout);
            if (ret > 0) {
                if (p[0] == DMR_MMDVM_FRAME_START) {
                    dmr_log_trace("mmdvm: got DMR_MMDVM_FRAME_START");
                    p++;
                    offset++;
                } else {
                    dmr_log_trace("mmdvm: expected DMR_MMDVM_FRAME_START, got 0x%02x", modem->buffer[0]);
                }
            }
            retry++;
            continue;
        } else if (offset == 1) {
            // Read packet size
            ret = dmr_serial_read(modem->fd, p, 1, timeout);
            dmr_log_trace("mmdvm: read %d/1 bytes", ret);
            if (ret > 0) {
                dmr_log_trace("mmdvm: got packet size %d", modem->buffer[1]);
                *length = modem->buffer[1];
                if (*length > 200) {
                    dmr_log_error("mmdvm: packet size %d too large", *length);
                    return dmr_mmdvm_error;
                }
            }
        } else {
            ret = dmr_serial_read(modem->fd, p, *length - offset, timeout);
            dmr_log_trace("mmdvm: read %d/%d bytes, length = %d", ret, *length - offset, *length);
        }
        if (ret <= 0) {
            modem->error = errno;
            dmr_log_error("mmdvm read failed: read(): %s\n", strerror(modem->error));
            return dmr_mmdvm_error;
        }

        offset += ret;
        p += ret;
        if (*length > 0 && offset >= *length)
            break;
    }

#ifdef DMR_MMDVM_DUMP_FILE
    fwrite(modem->buffer, *length, 1, dump);
    fflush(dump);
#endif
    return dmr_mmdvm_ok;
}

bool dmr_mmdvm_get_status(dmr_mmdvm_t *modem)
{
    if (modem == NULL || modem->fd == 0)
        return false;

    uint8_t buffer[3] = {DMR_MMDVM_FRAME_START, 3, DMR_MMDVM_GET_STATUS};
    return dmr_serial_write(modem->fd, &buffer, 3) == 3;
}

bool dmr_mmdvm_set_config(dmr_mmdvm_t *modem)
{
    if (modem == NULL || modem->fd == 0)
        return false;

    uint8_t buffer[10] = {
        DMR_MMDVM_FRAME_START,
        10,
        DMR_MMDVM_SET_CONFIG,
        0,
        0,
        modem->config.tx_delay,
        DMR_MMDVM_MODE_IDLE,
        modem->config.rx_level,
        modem->config.tx_level,
        modem->config.color_code
    };

    if (modem->config.rx_invert)
        buffer[3] |= 0b001;
    if (modem->config.tx_invert)
        buffer[3] |= 0b010;
    if (modem->config.ptt_invert)
        buffer[3] |= 0b100;
    if (modem->config.enable_dstar)
        buffer[4] |= 0b001;
    if (modem->config.enable_dmr)
        buffer[4] |= 0b010;
    if (modem->config.enable_ysf)
        buffer[4] |= 0b100;

    return dmr_serial_write(modem->fd, &buffer, 10) == 10;
}

bool dmr_mmdvm_set_mode(dmr_mmdvm_t *modem, uint8_t mode)
{
    if (modem == NULL || modem->fd == 0)
        return false;

    uint8_t buffer[4] = {DMR_MMDVM_FRAME_START, 4, DMR_MMDVM_SET_MODE, mode};
    return dmr_serial_write(modem->fd, &buffer, 4) == 4;
}

bool dmr_mmdvm_dmr_start(dmr_mmdvm_t *modem, bool tx)
{
    if (modem == NULL || modem->fd == 0)
        return false;

    uint8_t buffer[4] = {DMR_MMDVM_FRAME_START, 4, DMR_MMDVM_DMR_START, tx ? 1 : 0};
    return dmr_serial_write(modem->fd, &buffer, 4) == 4;
}

bool dmr_mmdvm_dmr_short_lc(dmr_mmdvm_t *modem, uint8_t lc[9])
{
    if (modem == NULL || modem->fd == 0)
        return false;

    uint8_t buffer[12] = {
        DMR_MMDVM_FRAME_START,
        12,
        DMR_MMDVM_DMR_SHORTLC,
        lc[0], lc[1], lc[2],
        lc[3], lc[4], lc[5],
        lc[6], lc[7], lc[8]
    };
    return dmr_serial_write(modem->fd, &buffer, 12) == 12;
}

int dmr_mmdvm_free(dmr_mmdvm_t *modem)
{
    int error = 0;
    if (modem == NULL)
        return 0;

#ifdef DMR_PLATFORM_WINDOWS
    if (modem->fd != NULL)
#else
    if (modem->fd > 0)
#endif
    {
        dmr_mmdvm_close(modem);
        error = modem->error;
    }

    if (modem->port != NULL) {
        dmr_free(modem->port);
    }

    dmr_free(modem);
    return error;
}

int dmr_mmdvm_close(dmr_mmdvm_t *modem)
{
    if (modem == NULL)
        return 0;
    if (modem->fd == 0)
        return 0;

    modem->error = 0;
    if (dmr_serial_close(modem->fd) != 0) {
        modem->error = dmr_serial_error();
        dmr_log_error(dmr_serial_error_message());
    }
    modem->fd = 0;
    return modem->error;
}
