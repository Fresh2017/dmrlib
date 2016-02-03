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
#include "dmr/payload/lc.h"
#include "dmr/payload/sync.h"
#include "dmr/proto.h"
#include "dmr/proto/mmdvm.h"
#include "dmr/thread.h"
#include "common/byte.h"
#include "common/serial.h"

static const char *dmr_mmdvm_proto_name = "mmdvm";
static struct {
    dmr_mmdvm_model_t model;
    uint16_t          flags;
} mmdvm_models[] = {
    { DMR_MMDVM_MODEL_GENERIC,
      DMR_MMDVM_HAS_ALL & ~DMR_MMDVM_HAS_SET_RF_CONFIG },
    { DMR_MMDVM_MODEL_G4KLX,
      DMR_MMDVM_HAS_ALL & ~DMR_MMDVM_HAS_SET_RF_CONFIG },
    { DMR_MMDVM_MODEL_DVMEGA,
      DMR_MMDVM_HAS_GET_VERSION | DMR_MMDVM_HAS_SET_RF_CONFIG | DMR_MMDVM_HAS_DMR },
    { DMR_MMDVM_MODEL_INVALID, 0 },
};

static uint16_t mmdvm_model_flags(dmr_mmdvm_model_t model)
{
    uint8_t i;
    for (i = 0; mmdvm_models[i].flags != 0; i++) {
        if (mmdvm_models[i].model == model) {
            return mmdvm_models[i].flags;
        }
    }
    return 0;
}

static int mmdvm_proto_init(void *modemptr)
{
    dmr_log_trace("mmdvm: init");
    dmr_mmdvm_t *modem = (dmr_mmdvm_t *)modemptr;
    if (modem == NULL)
        return dmr_error(DMR_EINVAL);

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
    dmr_thread_name_set("mmdvm");
    while (mmdvm_proto_active(modem)) {
        /*
        dmr_packet_t *out = dmr_mmdvm_queue_shift(modem);
        if (out != NULL) {
            if (dmr_mmdvm_send(modem, out) != 0) {
                dmr_log_error("mmdvm: send failed: %s", dmr_error_get());
                return -1;
            }
            //TALLOC_FREE(out);
            continue;
        }
        */

        uint32_t last_status_received = dmr_time_since(*modem->last_status_received);
        uint32_t last_status_requested = dmr_time_since(*modem->last_status_requested);
        dmr_log_debug("mmdvm: last status received %us, requested %us ago",
            last_status_received, last_status_requested);
        if (last_status_received > 3 && last_status_requested > 3) {
            dmr_log_debug("mmdvm: requesting modem status");
            // FIXME(PE1PLM): not implemented
            //dmr_mmdvm_get_status(modem);
            gettimeofday(modem->last_status_requested, NULL);
        }

        switch (dmr_mmdvm_poll(modem)) {
        case dmr_mmdvm_error:
            dmr_log_critical("mmdvm: stop on error: %s", strerror(modem->error));
            dmr_mutex_lock(modem->proto.mutex);
            modem->proto.is_active = false;
            dmr_mutex_unlock(modem->proto.mutex);
            break;
        case dmr_mmdvm_timeout:
        default:
            dmr_msleep(5);
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

    modem->proto.thread = talloc_zero(modem, dmr_thread_t);
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

    TALLOC_FREE(modem->proto.thread);
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

    /*
    if (dmr_mmdvm_queue(modem, packet) != 0) {
        dmr_log_error("mmdvm: failed to add packet to send queue: %s",
            dmr_error_get());
    }
    */
    if (dmr_mmdvm_send(modem, packet) != 0) {
        dmr_log_error("mmdvm: send: %s", dmr_error_get());
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

dmr_mmdvm_t *dmr_mmdvm_open(char *port, long baud, dmr_mmdvm_model_t model)
{
    dmr_mmdvm_t *modem = talloc_zero(NULL, dmr_mmdvm_t);
    if (modem == NULL)
        return NULL;

    memset(modem, 0, sizeof(dmr_mmdvm_t));

    // Setup MMDVM modem
    modem->version = 0;
    modem->firmware = NULL;
    modem->baud = baud;
    modem->model = model;
    modem->flags = mmdvm_model_flags(modem->model);
    if ((modem->flags & DMR_MMDVM_HAS_SET_MODE)) {
        modem->last_mode = DMR_MMDVM_MODE_INVALID;
    } else {
        modem->last_mode = DMR_MMDVM_MODE_DMR;
    }

    if ((modem->flags & DMR_MMDVM_HAS_DMR) == 0) {
        dmr_log_critical("mmdvm: modem does not support DMR");
        TALLOC_FREE(modem);
        return NULL;
    }

    dmr_ts_t ts;
    for (ts = DMR_TS1; ts < DMR_TS_INVALID; ts++) {
        modem->dmr_ts[ts].last_data_type = DMR_DATA_TYPE_INVALID;
    }

#ifdef DMR_MMDVM_DUMP_FILE
    dump = fopen("mmdvm.dump", "w+");
#endif

    // Setup MMDVM protocol
    modem->proto.name = dmr_mmdvm_proto_name;
    modem->proto.type = DMR_PROTO_MMDVM;
    modem->proto.init = mmdvm_proto_init;
    modem->proto.start = mmdvm_proto_start;
    modem->proto.stop = mmdvm_proto_stop;
    modem->proto.active = mmdvm_proto_active;
    modem->proto.tx = mmdvm_proto_tx;
    if (dmr_proto_mutex_init(&modem->proto) != 0) {
        dmr_log_error("modem: failed to init mutex");
        TALLOC_FREE(modem);
        return NULL;
    }

    // Setup send queue
    modem->queue_size = 32;
    modem->queue_used = 0;
    modem->queue_lock = talloc_zero(modem, dmr_mutex_t);
    modem->queue = talloc_size(modem, sizeof(dmr_packet_t) * modem->queue_size);

    if (modem->queue == NULL) {
        dmr_log_critical("mmdvm: out of memory");
        TALLOC_FREE(modem);
        return NULL;
    }

    // Book keeping
    modem->last_status_requested = talloc_zero(modem, struct timeval);
    modem->last_status_received = talloc_zero(modem, struct timeval);
    if (modem->last_status_received == NULL) {
        dmr_log_critical("mmdvm: out of memory");
        TALLOC_FREE(modem);
        return NULL;
    }


    // Setup serial port
    serial_t *serial = talloc_zero(modem, serial_t);
    if (serial == NULL) {
        dmr_log_critical("mmdvm: out of memory");
        TALLOC_FREE(modem);
        return NULL;
    }
    if ((modem->error = serial_by_name(port, &serial)) != 0) {
        dmr_log_error("mmdvm: serial by name: %s", strerror(errno));
        goto modem_error;
    }
    if ((modem->error = serial_open(serial, 'x')) != 0) {
        dmr_log_error("mmdvm: serial open %s failed",
            serial_name(serial));
        goto modem_error;
    }
    dmr_log_info("mmdvm: opened serial port %s", serial->name);
    if ((modem->error = serial_baudrate(serial, 115200)) != 0) {
        dmr_log_error("mmdvm: serial set baudrate on %s to 115200 failed",
            serial_name(serial));
        goto modem_error;
    }
    if ((modem->error = serial_parity(serial, SERIAL_PARITY_NONE)) != 0) {
        dmr_log_error("mmdvm: serial set parity on %s to NONE failed",
            serial_name(serial));
        goto modem_error;
    }
    if ((modem->error = serial_bits(serial, 8)) != 0) {
        dmr_log_error("mmdvm: serial set bits on %s to 8 failed",
            serial_name(serial));
        goto modem_error;
    }
    if ((modem->error = serial_stopbits(serial, 1)) != 0) {
        dmr_log_error("mmdvm: serial set stopbits on %s to 1 failed",
            serial_name(serial));
        goto modem_error;
    }
    if ((modem->error = serial_flowcontrol(serial, SERIAL_FLOWCONTROL_NONE)) != 0) {
        dmr_log_error("mmdvm: serial disable flow control on %s failed",
            serial_name(serial));
        goto modem_error;
    }
    if ((modem->error = serial_xon_xoff(serial, SERIAL_XON_XOFF_DISABLED)) != 0) {
        dmr_log_error("mmdvm: serial xon/xoff on %s to disabled failed",
            serial_name(serial));
        goto modem_error;
    }

    modem->serial = serial;
    goto done;

modem_error:
    if (modem->error != 0) {
        dmr_log_error("mmdvm: open %s failed: %s",
            serial_name(serial), strerror(modem->error));
        TALLOC_FREE(serial);
        dmr_mmdvm_free(modem);
        return NULL;
    }

done:
    return modem;
}

int dmr_mmdvm_poll(dmr_mmdvm_t *modem)
{
    if (modem == NULL || modem->serial == 0)
        return dmr_error(DMR_EINVAL);

    uint8_t len;
    uint16_t val[4];
    dmr_mmdvm_response_t res = dmr_mmdvm_get_response(modem, &len, 25, 1);

    dmr_ts_t ts = DMR_TS_INVALID;
    switch (res) {
    case dmr_mmdvm_timeout:
        return 0; // Nothing to do here

    case dmr_mmdvm_error:
        if (len == 0) {
            return 0; // Nothing to do here
        }

        dmr_log_debug("mmdvm: error reading response");
        return 1;

    case dmr_mmdvm_ok:
        if (len < 3) {
            dmr_log_warn("mmdvm: short reply received, skipped");
            return 0;
        }

        if (dmr_log_priority() <= DMR_LOG_PRIORITY_DEBUG) {
            dmr_dump_hex(modem->buffer, len);
        }

        dmr_mmdvm_header_t *header = (dmr_mmdvm_header_t *)modem->buffer;
        //length = modem->buffer[1];
        switch (header->command) {
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
                dmr_log_debug("mmdvm: data sync?");
                dmr_ts_t ts = DMR_TS1;
                dmr_sync_pattern_t pattern = dmr_sync_pattern_decode(packet);
                switch (pattern) {
                case DMR_SYNC_PATTERN_BS_SOURCED_VOICE:
                case DMR_SYNC_PATTERN_MS_SOURCED_VOICE:
                    packet->data_type = DMR_DATA_TYPE_VOICE_SYNC;
                    packet->meta.voice_frame = 0;
                    break;
                default:
                    if (dmr_slot_type_decode(packet) != 0) {
                        dmr_log_error("mmdvm: slot type decode failed: %s", dmr_error_get());
                        break;
                    }
                    switch (packet->data_type) {
                    case DMR_DATA_TYPE_VOICE_LC:
                        if (modem->dmr_ts[ts].last_data_type == packet->data_type)
                            break; /* Don't bother to read it again */

                        dmr_full_lc_t lc;
                        byte_zero(&lc, sizeof lc);
                        if (dmr_full_lc_decode(&lc, packet) != 0) {
                            dmr_log_error("mmdvm: full LC decode failed: %s", dmr_error_get());
                            break;
                        }
                        dmr_log_info("mmdvm: link control %u->%u, flco=%s",
                            lc.src_id, lc.dst_id, dmr_flco_pdu_name(lc.flco_pdu));

                        /* Book keeping, we need this to update the frames that will follow */
                        modem->dmr_ts[ts].last_src_id = lc.src_id;
                        modem->dmr_ts[ts].last_dst_id = lc.dst_id;
                        modem->dmr_ts[ts].last_flco = (lc.flco_pdu == DMR_FLCO_PDU_GROUP)
                            ? DMR_FLCO_GROUP : DMR_FLCO_PRIVATE;
                        modem->dmr_ts[ts].stream_id++;
                        modem->dmr_ts[ts].last_sequence = 0;
                        break;

                    case DMR_DATA_TYPE_TERMINATOR_WITH_LC:
                        break;
                    
                    case DMR_DATA_TYPE_DATA_HEADER:
                    case DMR_DATA_TYPE_MBC_HEADER:
                        /* TODO(pd0mz): parse LC */
                        /* For now, we are resetting the meta data */
                        modem->dmr_ts[ts].last_src_id = 0;
                        modem->dmr_ts[ts].last_dst_id = 0;
                        break;

                    case DMR_DATA_TYPE_CSBK:
                    case DMR_DATA_TYPE_MBC_CONTINUATION:
                    case DMR_DATA_TYPE_RATE12_DATA:
                    case DMR_DATA_TYPE_RATE34_DATA:
                    case DMR_DATA_TYPE_IDLE:
                        /* TODO(pd0mz): handle these types */
                        /* For now, we are resetting the meta data */
                        modem->dmr_ts[ts].last_src_id = 0;
                        modem->dmr_ts[ts].last_dst_id = 0;
                        dmr_log_warn("mmdvm: sending data is probably broken");
                        dmr_log_warn("mmdvm: not updating packet meta data for %s frame",
                            dmr_data_type_name(packet->data_type));
                        break;
                    case DMR_DATA_TYPE_INVALID:
                        if (modem->dmr_ts[ts].last_data_type == DMR_DATA_TYPE_VOICE_SYNC ||
                            modem->dmr_ts[ts].last_data_type == DMR_DATA_TYPE_VOICE) {
                            packet->data_type = DMR_DATA_TYPE_VOICE;
                            packet->meta.voice_frame = (++modem->dmr_ts[ts].last_voice_frame);
                        } else {
                            dmr_log_warn("mmdvm: unknown data type!");
                        }
                        break;
                    default:
                        dmr_log_warn("mmdvm: unsupported data type");
                        break;
                    }

                    if (modem->dmr_ts[ts].last_src_id != 0) {
                        packet->src_id = modem->dmr_ts[ts].last_src_id;
                        packet->dst_id = modem->dmr_ts[ts].last_dst_id;
                        packet->meta.sequence = modem->dmr_ts[ts].last_sequence % 0xff;
                        packet->meta.stream_id = modem->dmr_ts[ts].stream_id;
                        modem->dmr_ts[ts].last_sequence++;
                    } else {
                        dmr_log_warn("mmdvm: can't update %s frame, late entry not supported",
                            dmr_data_type_name(packet->data_type));
                    }
                    break;
                }
            } else {
                dmr_log_debug("mmdvm: %s", dmr_data_type_name(modem->buffer[3] & 0x0f));
                packet->data_type = (modem->buffer[3] & 0x0f);
            }

            // Receive callback(s)
            dmr_proto_rx_cb_run(&modem->proto, packet);

            // Cleanup
            TALLOC_FREE(packet);

            break;

        case DMR_MMDVM_DMR_LOST1:
        case DMR_MMDVM_DMR_LOST2:
            ts = (modem->buffer[3] & DMR_MMDVM_DMR_TS) > 0 ? DMR_TS2 : DMR_TS1;
            dmr_log_debug("mmdvm: DMR lost %u", dmr_ts_name(ts));

            // Expire our last packet received and run the timeout handler.
            modem->dmr_ts[ts].last_packet_received.tv_sec = 0;
            break;

        case DMR_MMDVM_GET_STATUS:
            dmr_log_debug("mmdvm: get status response");
            dmr_mmdvm_status_t *status = (dmr_mmdvm_status_t *)modem->buffer;
            dmr_log_info("mmdvm: state=%u, tx=%s, buffers={dstar=%u, dmr1=%u, dmr2=%u, ysf=%u}",
                status->state, DMR_LOG_BOOL((status->flags & 0x01) > 0),
                status->dstar_buffers, status->dmr_buffers[0], status->dmr_buffers[1], status->ysf_buffers);
            gettimeofday(modem->last_status_received, NULL);
            break;

        case DMR_MMDVM_GET_VERSION:
            dmr_log_debug("mmdvm: get version response");
            if (header->length < 4) {
                dmr_log_error("mmdvm: version response corrupt");
            } else {
                modem->buffer[header->length] = 0x00;
                dmr_log_info("mmdvm: version=%u, description=%s",
                    modem->buffer[3], &modem->buffer[4]);
            }
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
                    header->length - 3, &modem->buffer[3]);
                break;
            case DMR_MMDVM_DEBUG2:
                val[0] = (modem->buffer[header->length - 2] << 8) | modem->buffer[header->length - 1];
                dmr_log_error("mmdvm: debug2 %.*s %d\n",
                    header->length - 5, &modem->buffer[3], val[0]);
                break;
            case DMR_MMDVM_DEBUG3:
                val[0] = (modem->buffer[header->length - 4] << 8) | modem->buffer[header->length - 3];
                val[1] = (modem->buffer[header->length - 2] << 8) | modem->buffer[header->length - 1];
                dmr_log_error("mmdvm: debug3 %.*s %d %d\n",
                    header->length - 7, &modem->buffer[3], val[0], val[1]);
                break;
            case DMR_MMDVM_DEBUG4:
                val[0] = (modem->buffer[header->length - 6] << 8) | modem->buffer[header->length - 5];
                val[1] = (modem->buffer[header->length - 4] << 8) | modem->buffer[header->length - 3];
                val[2] = (modem->buffer[header->length - 2] << 8) | modem->buffer[header->length - 1];
                dmr_log_error("mmdvm: debug4 %.*s %d %d %d\n",
                    header->length - 9, &modem->buffer[3], val[0], val[1], val[2]);
                break;
            case DMR_MMDVM_DEBUG5:
                val[0] = (modem->buffer[header->length - 8] << 8) | modem->buffer[header->length - 7];
                val[1] = (modem->buffer[header->length - 6] << 8) | modem->buffer[header->length - 5];
                val[2] = (modem->buffer[header->length - 4] << 8) | modem->buffer[header->length - 3];
                val[3] = (modem->buffer[header->length - 2] << 8) | modem->buffer[header->length - 1];
                dmr_log_error("mmdvm: debug5 %.*s %d %d %d %d\n",
                    header->length - 11, &modem->buffer[3], val[0], val[1], val[2], val[3]);
                break;
            }
            break;

        case DMR_MMDVM_SAMPLES:
            dmr_log_debug("mmdvm: samples");
            break; // FIXME(maze): not implemented

        default:
            dmr_log_warn("mmdvm: modem sent unhandled response %d (%s), length %d\n",
                header->command, dmr_mmdvm_command_name(header->command),
                header->length);
            dmr_dump_hex(&modem->buffer[3], header->length - 2);
            break;
        }

        gettimeofday(&modem->last_packet_received, NULL);
    }

    return 0;
}

static int modem_read(dmr_mmdvm_t *modem, void *buf, size_t len, unsigned int timeout_ms)
{
    if (modem == NULL || modem->serial == NULL) {
        modem->error = EINVAL;
        return 0;
    }
    serial_t *serial = modem->serial;
    return serial_read(serial, buf, len, timeout_ms);
}

static int modem_write(dmr_mmdvm_t *modem, const void *buf, size_t len)
{
    if (modem == NULL || modem->serial == NULL) {
        modem->error = EINVAL;
        return 0;
    }
    serial_t *serial = modem->serial;
    return serial_write(serial, buf, len, 0);
}

int dmr_mmdvm_sync(dmr_mmdvm_t *modem)
{
    uint8_t i;

    if (modem == NULL)
        return dmr_error(DMR_EINVAL);

    dmr_log_debug("mmdvm: sync modem on %s", modem->port);
    for (i = 0; i < 6; i++) {
        uint8_t buf[3] = {DMR_MMDVM_FRAME_START, 3, DMR_MMDVM_GET_VERSION};
        if (modem_write(modem, buf, 3) != 3) {
            modem->error = errno;
            return dmr_error(DMR_EWRITE);
        }
        dmr_msleep(DMR_MMDVM_DELAY_MS);

        uint8_t len;
        dmr_mmdvm_response_t res = dmr_mmdvm_get_response(modem, &len, 1000, 16);
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
            modem->firmware = talloc_size(modem, len - 3);
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

int dmr_mmdvm_send(dmr_mmdvm_t *modem, dmr_packet_t *packet)
{
    uint8_t buf[37];
    switch (packet->data_type) {
    case DMR_DATA_TYPE_VOICE:
    case DMR_DATA_TYPE_VOICE_SYNC:
        dmr_log_debug("mmdvm: sending voice frames to modem");
        if (modem->last_mode != DMR_MMDVM_MODE_DMR) {
            if (!dmr_mmdvm_set_mode(modem, DMR_MMDVM_MODE_DMR)) {
                dmr_log_error("mmdvm: failed to send DMR start to modem");
            }
            modem->last_mode = DMR_MMDVM_MODE_DMR;
        }
        buf[0] = DMR_MMDVM_FRAME_START;
        buf[1] = 37;
        buf[2] = packet->ts == DMR_TS1 ? 0x18 : 0x1a;
        buf[3] = 0x20; // (packet->ts & 0x01) << 7;
        memcpy(&buf[4], packet->payload, 33);
        if (modem_write(modem, buf, sizeof(buf)) != sizeof(buf)) {
            dmr_log_error("mmdvm: failed to send %lu bytes of DMR data to modem",
                sizeof(buf));
        }
        break;

    /* XXX: not implemented on DVMEGA
    case DMR_DATA_TYPE_TERMINATOR_WITH_LC:
        if (modem->last_mode != DMR_MMDVM_MODE_DMR) {
            if (!dmr_mmdvm_set_mode(modem, DMR_MMDVM_MODE_DMR)) {
                dmr_log_error("mmdvm: failed to send DMR start to modem");
            }
            modem->last_mode = DMR_MMDVM_MODE_DMR;
        }
        dmr_log_trace("mmdvm: sending voice terminator to modem");
        buf[0] = DMR_MMDVM_FRAME_START;
        buf[1] = 4;
        buf[2] = (packet->ts == DMR_TS1) ? DMR_MMDVM_DMR_LOST1 : DMR_MMDVM_DMR_LOST2;
        if (dmr_serial_write(modem->fd, buf, 3) != 3) {
            dmr_log_error("mmdvm: failed to send DMR EOT");
        }
        //modem->last_mode = DMR_MMDVM_MODE_INVALID;
        break;
    */

    default:
        if (packet->data_type == DMR_DATA_TYPE_TERMINATOR_WITH_LC && (modem->flags & DMR_MMDVM_HAS_DMR_EOT)) {
            buf[0] = DMR_MMDVM_FRAME_START;
            buf[1] = 4;
            buf[2] = (packet->ts == DMR_TS1) ? DMR_MMDVM_DMR_LOST1 : DMR_MMDVM_DMR_LOST2;
            if (modem_write(modem, buf, 3) != 3) {
                dmr_log_error("mmdvm: failed to send DMR EOT");
            }
            break;
        }

        dmr_log_debug("mmdvm: sending %s to modem", dmr_data_type_name(packet->data_type));
        buf[0] = DMR_MMDVM_FRAME_START;
        buf[1] = 37;
        buf[2] = packet->ts == DMR_TS1 ? 0x18 : 0x1a;
        buf[3] = 0x40 | (packet->data_type & 0x0f);
        memcpy(&buf[4], packet->payload, 33);
        if (modem_write(modem, buf, sizeof(buf)) != sizeof(buf)) {
            dmr_log_error("mmdvm: failed to send %lu bytes of DMR data to modem",
                sizeof(buf));
        }
        break;
    }

    /* XXX: not implemented on DVMEGA
    if (packet->data_type == DMR_DATA_TYPE_VOICE_SYNC) {
        dmr_mmdvm_get_status(modem);
    }
    */

    return 0;
}

/** Add a packet to the send queue. */
int dmr_mmdvm_queue(dmr_mmdvm_t *modem, dmr_packet_t *packet_out)
{
    if (modem == NULL || packet_out == NULL)
        return dmr_error(DMR_EINVAL);

    //dmr_mutex_lock(modem->queue_lock);
    int ret = 0;
    if (modem->queue_used + 1 < modem->queue_size) {
        dmr_packet_t *packet = talloc_zero(NULL, dmr_packet_t);
        if (packet == NULL) {
            dmr_log_critical("mmdvm: out of memory");
            ret = dmr_error(DMR_ENOMEM);
            goto bail;
        }
        memcpy(packet, packet_out, sizeof(dmr_packet_t));
        modem->queue[modem->queue_used++] = packet;
    } else {
        ret = dmr_error_set("mmdvm: send queue full");
        goto bail;
    }

bail:
    //dmr_mutex_unlock(modem->queue_lock);

    return ret;
}

dmr_packet_t *dmr_mmdvm_queue_shift(dmr_mmdvm_t *modem)
{
    if (modem == NULL)
        return NULL;

    dmr_packet_t *packet = NULL;
    //dmr_mutex_lock(modem->queue_lock);
    if (modem->queue_used) {
        packet = modem->queue[0];
        modem->queue_used--;
        size_t i;
        for (i = 1; i < modem->queue_used; i++) {
            modem->queue[i - 1] = modem->queue[i];
        }
    }
    //dmr_mutex_unlock(modem->queue_lock);

    return packet;
}

dmr_mmdvm_response_t dmr_mmdvm_get_response(dmr_mmdvm_t *modem, uint8_t *length, unsigned int timeout_ms, int retries)
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
            ret = modem_read(modem, p, 1, timeout_ms);
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
            ret = modem_read(modem, p, 1, timeout_ms);
            //dmr_log_trace("mmdvm: read %d/1 bytes", ret);
            if (ret > 0) {
                dmr_log_trace("mmdvm: got packet size %d", modem->buffer[1]);
                *length = modem->buffer[1];
                if (*length > 200) {
                    dmr_log_error("mmdvm: packet size %d too large", *length);
                    return dmr_mmdvm_error;
                }
            }
        } else {
            ret = modem_read(modem, p, *length - offset, timeout_ms);
            dmr_log_trace("mmdvm: read %d/%d bytes, length = %d", ret, *length - offset, *length);
        }
        if (ret <= 0) {
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
    if (modem == NULL || modem->serial == NULL || (modem->flags & DMR_MMDVM_HAS_GET_STATUS) == 0)
        return false;

    dmr_mmdvm_header_t req = {
        DMR_MMDVM_FRAME_START,
        3,
        DMR_MMDVM_GET_STATUS
    };
    return modem_write(modem, (uint8_t *)&req, req.length) == req.length;
}

bool dmr_mmdvm_get_version(dmr_mmdvm_t *modem)
{
    if (modem == NULL || modem->serial == NULL || (modem->flags & DMR_MMDVM_HAS_GET_VERSION) == 0)
        return false;

    dmr_mmdvm_header_t req = {
        DMR_MMDVM_FRAME_START,
        3,
        DMR_MMDVM_GET_VERSION
    };
    return modem_write(modem, (uint8_t *)&req, req.length) == req.length;
}

bool dmr_mmdvm_set_config(dmr_mmdvm_t *modem)
{
    if (modem == NULL || modem->serial == NULL || (modem->flags & DMR_MMDVM_HAS_SET_CONFIG) == 0)
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
        buffer[3] |= 0x01;
    if (modem->config.tx_invert)
        buffer[3] |= 0x02;
    if (modem->config.ptt_invert)
        buffer[3] |= 0x04;
    if (modem->config.enable_dstar)
        buffer[4] |= 0x01;
    if (modem->config.enable_dmr)
        buffer[4] |= 0x02;
    if (modem->config.enable_ysf)
        buffer[4] |= 0x04;

    return modem_write(modem, &buffer, 10) == 10;
}

bool dmr_mmdvm_set_mode(dmr_mmdvm_t *modem, uint8_t mode)
{
    if (modem == NULL || modem->serial == NULL || (modem->flags & DMR_MMDVM_HAS_SET_MODE) == 0)
        return false;

    uint8_t buffer[4] = {DMR_MMDVM_FRAME_START, 4, DMR_MMDVM_SET_MODE, mode};
    return modem_write(modem, &buffer, 4) == 4;
}

bool dmr_mmdvm_set_rf_config(dmr_mmdvm_t *modem, uint32_t rx_freq, uint32_t tx_freq)
{
    if (modem == NULL || modem->serial == NULL || (modem->flags & DMR_MMDVM_HAS_SET_RF_CONFIG) == 0)
        return false;

    dmr_log_info("mmdvm: tuning RF to %u/%u Hz", rx_freq, tx_freq);
    uint8_t buffer[12] = {
        DMR_MMDVM_FRAME_START,
        12,
        DMR_MMDVM_SET_RF_CONFIG,
        0x00, /* flags, reserved */
        rx_freq >> 24,
        rx_freq >> 16,
        rx_freq >> 8,
        rx_freq,
        tx_freq >> 24,
        tx_freq >> 16,
        tx_freq >> 8,
        tx_freq
    };
    return modem_write(modem, &buffer, 12) == 12;
}

bool dmr_mmdvm_dmr_start(dmr_mmdvm_t *modem, bool tx)
{
    if (modem == NULL || modem->serial == NULL)
        return false;

    uint8_t buffer[4] = {DMR_MMDVM_FRAME_START, 4, DMR_MMDVM_DMR_START, tx ? 1 : 0};
    return modem_write(modem, &buffer, 4) == 4;
}

bool dmr_mmdvm_dmr_short_lc(dmr_mmdvm_t *modem, uint8_t lc[9])
{
    if (modem == NULL || modem->serial == NULL || (modem->flags & DMR_MMDVM_HAS_DMR_SHORT_LC) == 0)
        return false;

    uint8_t buffer[12] = {
        DMR_MMDVM_FRAME_START,
        12,
        DMR_MMDVM_DMR_SHORTLC,
        lc[0], lc[1], lc[2],
        lc[3], lc[4], lc[5],
        lc[6], lc[7], lc[8]
    };
    return modem_write(modem, &buffer, 12) == 12;
}

int dmr_mmdvm_free(dmr_mmdvm_t *modem)
{
    int error = 0;
    if (modem == NULL)
        return 0;

    if (modem->serial != NULL)
    {
        dmr_mmdvm_close(modem);
        error = modem->error;
    }

    if (modem->port != NULL) {
        TALLOC_FREE(modem->port);
    }

    TALLOC_FREE(modem);
    return error;
}

int dmr_mmdvm_close(dmr_mmdvm_t *modem)
{
    if (modem == NULL)
        return 0;
    if (modem->port == NULL)
        return 0;

    modem->error = 0;
    serial_t *serial = (serial_t *)modem->serial;
    if (serial_close(serial) != 0) {
        modem->error = errno;
        dmr_log_error("mmdvm: close failed: %s", strerror(errno));
    }
    TALLOC_FREE(modem->serial);
    return modem->error;
}
