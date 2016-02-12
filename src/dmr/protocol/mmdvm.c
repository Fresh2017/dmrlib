#include "dmr/error.h"
#include "dmr/malloc.h"
#include "dmr/id.h"
#include "dmr/log.h"
#include "dmr/protocol.h"
#include "dmr/protocol/mmdvm.h"
#include "common/byte.h"
#include "common/serial.h"

DMR_PRV static struct { dmr_mmdvm_command command; const char *name; } commands[] = {
    { DMR_MMDVM_GET_VERSION,   "get version"   },
    { DMR_MMDVM_GET_STATUS,    "get status"    },
    { DMR_MMDVM_SET_CONFIG,    "set config"    },
    { DMR_MMDVM_SET_MODE,      "set mode"      },
    { DMR_MMDVM_SET_RF_CONFIG, "set rf config" },
    { DMR_MMDVM_DSTAR_HEADER,  "D-STAR header" },
    { DMR_MMDVM_DSTAR_DATA,    "D-STAR data"   },
    { DMR_MMDVM_DSTAR_LOST,    "D-STAR lost"   },
    { DMR_MMDVM_DSTAR_EOT,     "D-STAR EOT"    },
    { DMR_MMDVM_DMR_DATA1,     "DMR data TS1"  },
    { DMR_MMDVM_DMR_LOST1,     "DMR lost TS1"  },
    { DMR_MMDVM_DMR_DATA2,     "DMR data TS2"  },
    { DMR_MMDVM_DMR_LOST2,     "DMR lost TS2"  },
    { DMR_MMDVM_DMR_SHORTLC,   "DMR short LC"  },
    { DMR_MMDVM_DMR_START,     "DMR start"     },
    { DMR_MMDVM_YSF_DATA,      "YSF data"      },
    { DMR_MMDVM_YSF_LOST,      "YSF lost"      },
    { DMR_MMDVM_ACK,           "ACK"           },
    { DMR_MMDVM_NAK,           "NAK"           },
    { DMR_MMDVM_DUMP,          "DUMP"          },
    { DMR_MMDVM_DEBUG1,        "DEBUG1"        },
    { DMR_MMDVM_DEBUG2,        "DEBUG2"        },
    { DMR_MMDVM_DEBUG3,        "DEBUG3"        },
    { DMR_MMDVM_DEBUG4,        "DEBUG4"        },
    { DMR_MMDVM_DEBUG5,        "DEBUG5"        },
    { 0,                       NULL            }
};

DMR_API const char *dmr_mmdvm_command_name(dmr_mmdvm_command command)
{
    int i;
    for (i = 0; commands[i].name != NULL; i++) {
        if (commands[i].command == command) {
            return commands[i].name;
        }
    }
    return "unknown";
}

DMR_PRV static struct { dmr_mmdvm_reason reason; const char *name; } reasons[] = {
    { DMR_MMDVM_INVALID_VALUE,    "invalid command value"   },
    { DMR_MMDVM_WRONG_MODE,       "wrong mode"              },
    { DMR_MMDVM_COMMAND_TOO_LONG, "command too long"        },
    { DMR_MMDVM_DATA_INCORRECT,   "data incorrect"          },
    { DMR_MMDVM_NOT_ENOUGH_SPACE, "not enough buffer space" },
    { 0,                          NULL                      }
};

DMR_API const char *dmr_mmdvm_reason_name(dmr_mmdvm_reason reason)
{
    int i;
    for (i = 0; reasons[i].name != NULL; i++) {
        if (reasons[i].reason == reason) {
            return reasons[i].name;
        }
    }
    return "unknown";
}

DMR_PRV static struct { dmr_mmdvm_model model; const char *name; } models[] = {
    { DMR_MMDVM_MODEL_G4KLX,  "MMDVM (G4KLX)"  },
    { DMR_MMDVM_MODEL_DVMEGA, "DVMEGA"         },
    { 0,                      NULL             }
};

DMR_API const char *dmr_mmdvm_model_name(dmr_mmdvm_model model)
{
    int i;
    for (i = 0; models[i].name != NULL; i++) {
        if (models[i].model == model) {
            return models[i].name;
        }
    }
    return "unknown";
}

DMR_PRV static struct { dmr_mmdvm_state state; const char *name; } states[] = {
    { DMR_MMDVM_STATE_IDLE,      "idle"          },
    { DMR_MMDVM_STATE_DSTAR,     "D-STAR"        },
    { DMR_MMDVM_STATE_DMR,       "DMR"           },
    { DMR_MMDVM_STATE_YSF,       "System Fusion" },
    { DMR_MMDVM_STATE_CALIBRATE, "calibration"   },
    { 0,                         NULL            }
};

DMR_API const char *dmr_mmdvm_state_name(dmr_mmdvm_state state)
{
    int i;
    for (i = 0; states[i].name != NULL; i++) {
        if (states[i].state == state) {
            return states[i].name;
        }
    }
    return "unknown";
}

DMR_API dmr_mmdvm *dmr_mmdvm_new(const char *port, int baud, dmr_mmdvm_model model, dmr_color_code color_code)
{
    DMR_MALLOC_CHECK(dmr_mmdvm, mmdvm);
    DMR_NULL_CHECK_FREE(mmdvm->id = dmr_palloc_size(mmdvm, 64), mmdvm);

    /* Setup ident */
#if defined(DMR_PLATFORM_WINDOWS)
    snprintf(mmdvm->id, 64, "mmdvm[%s]", strrchr(port, '\\') + 1);
#else
    snprintf(mmdvm->id, 64, "mmdvm[%s]", strrchr(port, '/') + 1);
#endif

    /* Setup queues */
    mmdvm->trq = dmr_rawq_new(32);
    DMR_NULL_CHECK_FREE(mmdvm->trq, mmdvm);

    /* Setup serial port */
    if ((mmdvm->port = talloc_strdup(mmdvm, port)) == NULL) {
        DMR_MM_FATAL("out of memory");
        TALLOC_FREE(mmdvm);
        return NULL;
    }
    mmdvm->baud = baud;
    serial_t *serial = talloc_zero(mmdvm, serial_t);
    if (serial == NULL) {
        DMR_MM_FATAL("out of memory");
        TALLOC_FREE(mmdvm);
        return NULL;
    }
    if (serial_by_name(port, &serial) != 0) {
        DMR_MM_FATAL("can't find port %s: %s", port, strerror(errno));
        TALLOC_FREE(mmdvm);
        return NULL;
    }

    mmdvm->started = false;
    mmdvm->serial = serial;
    mmdvm->model = model;
    mmdvm->color_code = color_code;

    /* clear ack buffer */
    byte_zero((void *)mmdvm->ack, sizeof(mmdvm->ack));

    return mmdvm;
}

/** Start the modem communications.
 * If an error occurs setting up the serial communications, the modem will
 * be freed and the caller needs to run dmr_mmdvm_new again. */
DMR_API int dmr_mmdvm_start(dmr_mmdvm *mmdvm)
{
    DMR_ERROR_IF_NULL(mmdvm, DMR_EINVAL);
    
    if (mmdvm->started) {
        DMR_MM_INFO("start (ignored, already running)");
        return 0;
    }

    serial_t *serial = (serial_t *)mmdvm->serial;
    DMR_MM_INFO("start on %s (%u baud, 8n1)",
        serial_name(serial), mmdvm->baud);
    if (serial_open(serial, 'x') != 0) {
        goto error;
    }
#define TRY_SERIAL(op, val) if (serial_##op(serial, val) != 0) goto error
    TRY_SERIAL(baudrate, mmdvm->baud);
    TRY_SERIAL(bits, 8);
    TRY_SERIAL(parity, SERIAL_PARITY_NONE);
    TRY_SERIAL(stopbits, 1);
    TRY_SERIAL(flowcontrol, SERIAL_FLOWCONTROL_NONE);
    TRY_SERIAL(xon_xoff, SERIAL_XON_XOFF_DISABLED);
#undef TRY_SERIAL

    int ret;
    if ((ret = dmr_mmdvm_set_mode(mmdvm, DMR_MMDVM_MODE_DMR)) != 0)
        goto error;

    if ((ret = dmr_mmdvm_set_config(mmdvm, 0, DMR_MMDVM_MODE_DMR, 0, DMR_MMDVM_STATE_DMR, 0, 0, mmdvm->color_code)) != 0)
        goto error;

    DMR_MM_INFO("modem started");
    mmdvm->started = true;
    return 0;

error:
    DMR_MM_ERROR("serial open %s failed: %s",
        serial_name(serial), strerror(errno));
    TALLOC_FREE(serial);
    TALLOC_FREE(mmdvm);
    return dmr_error(DMR_EINVAL);
}

DMR_API int dmr_mmdvm_parse_frame(dmr_mmdvm *mmdvm, dmr_mmdvm_frame frame, dmr_parsed_packet **parsed_out)
{
    DMR_ERROR_IF_NULL(mmdvm, DMR_EINVAL);
    DMR_ERROR_IF_NULL(frame, DMR_EINVAL);

    if (frame[0] != DMR_MMDVM_FRAME_START) {
        DMR_MM_DEBUG("no frame start, can't parse");
        return -1;
    }

    DMR_MM_DEBUG("parse %u bytes %s command (%#02x)",
        mmdvm->frame[1], dmr_mmdvm_command_name(mmdvm->frame[2]), mmdvm->frame[2]);

    dmr_parsed_packet *parsed;
    switch (frame[2]) {
    case DMR_MMDVM_GET_VERSION:
        if (mmdvm->description) {
            TALLOC_FREE(mmdvm->description);
        }
        mmdvm->protocol_version = frame[3];
        mmdvm->description = talloc_zero_size(mmdvm, frame[1] - 3);
        byte_copy(mmdvm->description, frame + 4, frame[1] - 4);
        return 0;

    case DMR_MMDVM_GET_STATUS:
        mmdvm->modes = frame[3];
        mmdvm->status = frame[4];
        mmdvm->tx_on = (bool)frame[5];
        byte_copy(mmdvm->buffer_size, frame + 6, 4);
        DMR_MM_DEBUG("status: dmr=%u, state=%s, tx=%s, TS1 buf=%u, TS2 buf=%u",
            (mmdvm->modes & DMR_MMDVM_MODE_DMR) >> 1,
            dmr_mmdvm_state_name(mmdvm->status),
            mmdvm->tx_on ? "on" : "off",
            mmdvm->buffer_size[DMR_MMDVM_BUFSIZE_DMR_TS1],
            mmdvm->buffer_size[DMR_MMDVM_BUFSIZE_DMR_TS2]);
        break;

    case DMR_MMDVM_DMR_DATA1:
    case DMR_MMDVM_DMR_DATA2:
        if ((parsed = dmr_packet_decode(frame + 4)) == NULL) {
            return dmr_error(DMR_ENOMEM);
        }
        parsed->ts = frame[3] == 0x1a;
        *parsed_out = parsed;
        DMR_MM_DEBUG("received DMR packet on %s %u->%u",
            dmr_ts_name(parsed->ts), parsed->src_id, parsed->dst_id);
        break;

    case DMR_MMDVM_ACK:
        DMR_MM_INFO("modem sent ACK in reply to %s",
            dmr_mmdvm_command_name(mmdvm->frame[3]));
        mmdvm->ack[mmdvm->frame[3]] = true;
#if defined(DMR_DEBUG)
        dmr_dump_hex((void *)mmdvm->ack, sizeof(mmdvm->ack));
#endif
        break;

    case DMR_MMDVM_NAK:
        DMR_MM_WARN("modem sent NAK in reply to %s, reason: %s",
            dmr_mmdvm_command_name(mmdvm->frame[3]),
            dmr_mmdvm_reason_name(mmdvm->frame[4]));
        break;

    default:
        DMR_MM_WARN("modem sent unknown command %#02x", frame[2]);
        break;
    }
    return 0;
}

DMR_API int dmr_mmdvm_read(dmr_mmdvm *mmdvm, dmr_parsed_packet **parsed_out)
{
    serial_t *serial = (serial_t *)mmdvm->serial;
    bool again = true;
    int ret, retry;
    size_t len = DMR_MMDVM_FRAME_MAX - mmdvm->pos;

read_again:
    errno = 0;
    retry = 0;
    do {
        ret = serial_read_nonblock(serial, mmdvm->frame + mmdvm->pos, len);
        if (ret <= 0 && errno != 0) {
            DMR_MM_ERROR("read: %s", strerror(errno));
        }
        if (++retry > 16)
            break;
    } while (ret <= 0 && (errno == EAGAIN || errno == EINTR));
    if (ret <= 0)
        return ret;

#if defined(DMR_DEBUG)
    dmr_dump_hex(mmdvm->frame + mmdvm->pos, ret);
#endif

    /* Shift the buffer until we have a FRAME_START byte */
    mmdvm->pos += ret;
    while (mmdvm->frame[0] != DMR_MMDVM_FRAME_START &&
           mmdvm->pos != 0) {
        memmove(mmdvm->frame, mmdvm->frame + 1, DMR_MMDVM_FRAME_MAX);
        mmdvm->pos--;
    }

    DMR_MM_DEBUG("buffer pos %lu", mmdvm->pos);
    
    /* If the frame length doesn't match, wait for the next read */
    if (mmdvm->pos < 3) {
        DMR_MM_DEBUG("waiting for more data");
        if (again) {
            again = false;
            goto read_again;
        }
        return 0;
    }
    if (mmdvm->pos < mmdvm->frame[1]) {
        DMR_MM_DEBUG("not enough data received, at %u/%u", mmdvm->pos, mmdvm->frame[1]);
        return 0;
    }

    ret = dmr_mmdvm_parse_frame(mmdvm, mmdvm->frame, parsed_out);

    /* Shift the buffer */
    size_t i;
    len = mmdvm->frame[1];
    for (i = 0; i < len; i++) {
        mmdvm->frame[i] = mmdvm->frame[i + len];
    }
    byte_zero(mmdvm->frame + len, DMR_MMDVM_FRAME_MAX - len);
        
    if (mmdvm->pos >= len)
        mmdvm->pos -= len;
    else
        mmdvm->pos = 0;

    return ret;
}

DMR_API int dmr_mmdvm_write(dmr_mmdvm *mmdvm)
{
    DMR_ERROR_IF_NULL(mmdvm, DMR_EINVAL);

    int ret = 0;
    while (!dmr_rawq_empty(mmdvm->trq)) {
        dmr_raw *raw = dmr_rawq_shift(mmdvm->trq);
        if (raw == NULL)
            continue;

        DMR_MM_DEBUG("send %u bytes %s command (%#02x)",
            raw->buf[1], dmr_mmdvm_command_name(raw->buf[2]), raw->buf[2]);

        if ((ret = dmr_mmdvm_send_raw(mmdvm, raw)) == -1)
            break;
    }

    return ret;
}

DMR_API int dmr_mmdvm_send(dmr_mmdvm *mmdvm, dmr_parsed_packet *parsed)
{
    DMR_ERROR_IF_NULL(mmdvm, DMR_EINVAL);
    DMR_ERROR_IF_NULL(parsed, DMR_EINVAL);

    const char *src_call = dmr_id_name(parsed->src_id);
    const char *dst_call = dmr_id_name(parsed->dst_id);
    DMR_MM_DEBUG("%s/%02x, type=%s from %u(%s)->%u(%s), privacy=%u, stream=%08x",
        dmr_ts_name(parsed->ts), parsed->sequence,
        dmr_data_type_name(parsed->data_type),
        parsed->src_id, src_call == NULL ? "?" : src_call,
        parsed->dst_id, dst_call == NULL ? "?" : dst_call,
        parsed->flco, parsed->stream_id);

    uint8_t control = 0;
    switch (dmr_sync_pattern_decode(parsed->packet)) {
    case DMR_SYNC_PATTERN_BS_SOURCED_VOICE:
    case DMR_SYNC_PATTERN_MS_SOURCED_VOICE:
        control |= 0x20;
        break;
    case DMR_SYNC_PATTERN_BS_SOURCED_DATA:
    case DMR_SYNC_PATTERN_MS_SOURCED_DATA:
        control |= 0x40;
        control |= (parsed->data_type & 0x0f);
        break;
    default:
        break;
    }

    /* DVMEGA works on TS2 */
    if (mmdvm->model == DMR_MMDVM_MODEL_DVMEGA) {
        parsed->ts = DMR_TS2;
        if (parsed->data_type == DMR_DATA_TYPE_VOICE) {
            control |= 0x20;
        }
    }

    /* construct 37 byte DMR frame */
    dmr_raw *raw = dmr_raw_new(37); /* malloc, freed by send_raw */
    DMR_ERROR_IF_NULL(raw, DMR_ENOMEM);
    dmr_raw_add_uint8(raw, DMR_MMDVM_FRAME_START);
    dmr_raw_add_uint8(raw, 37);
    dmr_raw_add_uint8(raw, parsed->ts == DMR_TS1
        ? DMR_MMDVM_DMR_DATA1
        : DMR_MMDVM_DMR_DATA2);
    dmr_raw_add_uint8(raw, control);
    dmr_raw_add(raw, parsed->packet, DMR_PACKET_LEN);
    return dmr_mmdvm_send_raw(mmdvm, raw);
    //return dmr_rawq_add(mmdvm->trq, raw);
}

DMR_API int dmr_mmdvm_send_raw(dmr_mmdvm *mmdvm, dmr_raw *raw)
{
    DMR_ERROR_IF_NULL(mmdvm, DMR_EINVAL);
    DMR_ERROR_IF_NULL(raw, DMR_EINVAL);

#if defined(DMR_DEBUG)
    dmr_dump_hex(raw->buf, raw->len);
#endif

    serial_t *serial = (serial_t *)mmdvm->serial;
    int ret = 0, retry = 0;
    size_t pos = 0;
    do {
        ret = serial_write_nonblock(serial, raw->buf + pos, raw->len - pos);
        if (ret > 0) {
            pos += ret;
            DMR_MM_DEBUG("sent %u/%d", pos, raw->len);
        } else if (++retry > 16) {
            DMR_MM_ERROR("send(%u) failed after %d retries: %s", raw->len, retry, strerror(errno));
            return -1;
        }
    } while (ret == -1 && (errno == EINVAL || errno == EAGAIN));
    if (ret == -1) {
        DMR_MM_ERROR("send(%u) failed: %s", raw->len, strerror(errno));
    } else {
        ret = 0;
    }

    /* Every 6th DMR data frame we request the modem status to check if there
     * is sufficient buffer size. */
    if (raw->len > 3 && (raw->buf[2] == DMR_MMDVM_DMR_DATA2)) {
        if ((++mmdvm->sent) % 6 == 0) {
            dmr_mmdvm_get_status(mmdvm);
            dmr_mmdvm_get_version(mmdvm);
        }
    }

    dmr_raw_free(raw);
    return ret;
}

DMR_API int dmr_mmdvm_get_status(dmr_mmdvm *mmdvm)
{
    DMR_ERROR_IF_NULL(mmdvm, DMR_EINVAL);

    dmr_raw *raw = dmr_raw_new(3);
    DMR_ERROR_IF_NULL(raw, DMR_ENOMEM);
    dmr_raw_add_uint8(raw, DMR_MMDVM_FRAME_START);
    dmr_raw_add_uint8(raw, 3);
    dmr_raw_add_uint8(raw, DMR_MMDVM_GET_STATUS);
    return dmr_mmdvm_send_raw(mmdvm, raw);
    //return dmr_rawq_add(mmdvm->trq, raw);
}

DMR_API int dmr_mmdvm_get_version(dmr_mmdvm *mmdvm)
{
    DMR_ERROR_IF_NULL(mmdvm, DMR_EINVAL);

    dmr_raw *raw = dmr_raw_new(3);
    DMR_ERROR_IF_NULL(raw, DMR_ENOMEM);
    dmr_raw_add_uint8(raw, DMR_MMDVM_FRAME_START);
    dmr_raw_add_uint8(raw, 3);
    dmr_raw_add_uint8(raw, DMR_MMDVM_GET_VERSION);
    return dmr_mmdvm_send_raw(mmdvm, raw);
    //return dmr_rawq_add(mmdvm->trq, raw);
}

DMR_API int dmr_mmdvm_set_config(dmr_mmdvm *mmdvm, uint8_t invert, uint8_t mode, uint8_t delay_ms, dmr_mmdvm_state state, uint8_t rx_level, uint8_t tx_level, dmr_color_code color_code)
{
    DMR_ERROR_IF_NULL(mmdvm, DMR_EINVAL);

    dmr_raw *raw = dmr_raw_new(10);
    DMR_ERROR_IF_NULL(raw, DMR_ENOMEM);
    dmr_raw_add_uint8(raw, DMR_MMDVM_FRAME_START);
    dmr_raw_add_uint8(raw, 10);
    dmr_raw_add_uint8(raw, DMR_MMDVM_SET_CONFIG);
    dmr_raw_add_uint8(raw, invert);
    dmr_raw_add_uint8(raw, mode);
    dmr_raw_add_uint8(raw, delay_ms);
    dmr_raw_add_uint8(raw, state);
    dmr_raw_add_uint8(raw, rx_level);
    dmr_raw_add_uint8(raw, tx_level);
    dmr_raw_add_uint8(raw, color_code);
    return dmr_mmdvm_send_raw(mmdvm, raw);
    //return dmr_rawq_add(mmdvm->trq, raw);
} 

DMR_API int dmr_mmdvm_set_rf_config(dmr_mmdvm *mmdvm, uint32_t rx_freq, uint32_t tx_freq)
{
    DMR_ERROR_IF_NULL(mmdvm, DMR_EINVAL);

    if (mmdvm->model != DMR_MMDVM_MODEL_DVMEGA) {
        DMR_MM_WARN("set RF config not supported by model \"%s\"",
            dmr_mmdvm_model_name(mmdvm->model));
        return -1;
    }

    if (mmdvm->rx_freq != rx_freq || mmdvm->tx_freq != tx_freq) {
        DMR_MM_DEBUG("reset RF config ACK state, rx %u->%u, tx %u->%u",
            mmdvm->rx_freq, rx_freq,
            mmdvm->tx_freq, tx_freq);
        mmdvm->ack[DMR_MMDVM_SET_RF_CONFIG] = false;
        mmdvm->rx_freq = rx_freq;
        mmdvm->tx_freq = tx_freq;
    }

    dmr_raw *raw = dmr_raw_new(12);
    DMR_ERROR_IF_NULL(raw, DMR_ENOMEM);
    dmr_raw_add_uint8(raw, DMR_MMDVM_FRAME_START);
    dmr_raw_add_uint8(raw, 12);
    dmr_raw_add_uint8(raw, DMR_MMDVM_SET_RF_CONFIG);
    dmr_raw_add_uint8(raw, 0); /* flags, reserved */
    dmr_raw_add_uint32(raw, rx_freq);
    dmr_raw_add_uint32(raw, tx_freq);
    return dmr_mmdvm_send_raw(mmdvm, raw);
    //return dmr_rawq_add(mmdvm->trq, raw);
}

DMR_API int dmr_mmdvm_set_mode(dmr_mmdvm *mmdvm, dmr_mmdvm_mode mode)
{
    DMR_ERROR_IF_NULL(mmdvm, DMR_EINVAL);
    
    dmr_raw *raw = dmr_raw_new(4);
    DMR_ERROR_IF_NULL(raw, DMR_ENOMEM);
    dmr_raw_add_uint8(raw, DMR_MMDVM_FRAME_START);
    dmr_raw_add_uint8(raw, 4);
    dmr_raw_add_uint8(raw, DMR_MMDVM_SET_MODE);
    dmr_raw_add_uint8(raw, mode);
    return dmr_mmdvm_send_raw(mmdvm, raw);
    //return dmr_rawq_add(mmdvm->trq, raw);
}

DMR_API int dmr_mmdvm_close(dmr_mmdvm *mmdvm)
{
    DMR_ERROR_IF_NULL(mmdvm, DMR_EINVAL);

    serial_t *serial = (serial_t *)mmdvm->serial;
    if (serial != NULL) {
        serial_close(serial);
        serial_free(serial);
    }
    TALLOC_FREE(mmdvm);
    return 0;
}
