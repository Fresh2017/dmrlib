#include "dmr/error.h"
#include "dmr/malloc.h"

#include "dmr/log.h"
#include "dmr/protocol.h"
#include "dmr/protocol/mmdvm.h"
#include "common/byte.h"
#include "common/serial.h"

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

DMR_API dmr_mmdvm *dmr_mmdvm_new(const char *port, int baud, dmr_mmdvm_model model)
{
    DMR_MALLOC_CHECK(dmr_mmdvm, mmdvm);

    /* Setup queues */
    DMR_NULL_CHECK_FREE(mmdvm->rxq = dmr_packetq_new(), mmdvm);
    DMR_NULL_CHECK_FREE(mmdvm->txq = dmr_packetq_new(), mmdvm);
    STAILQ_INIT(&mmdvm->frameq);

    /* Setup serial port */
    if ((mmdvm->port = talloc_strdup(mmdvm, port)) == NULL) {
        dmr_log_critical("mmdvm: out of memory");
        TALLOC_FREE(mmdvm);
        return NULL;
    }
    mmdvm->baud = baud;
    serial_t *serial = talloc_zero(mmdvm, serial_t);
    if (serial == NULL) {
        dmr_log_critical("mmdvm: out of memory");
        TALLOC_FREE(mmdvm);
        return NULL;
    }
    if (serial_by_name(port, &serial) != 0) {
        dmr_log_critical("mmdvm: can't find port %s: %s", port, strerror(errno));
        TALLOC_FREE(mmdvm);
        return NULL;
    }

    mmdvm->serial = serial;
    mmdvm->model = model;

    return mmdvm;
}

/** Start the modem communications.
 * If an error occurs setting up the serial communications, the modem will
 * be freed and the caller needs to run dmr_mmdvm_new again. */
DMR_API int dmr_mmdvm_start(dmr_mmdvm *mmdvm)
{
    if (mmdvm == NULL)
        return dmr_error(DMR_EINVAL);

    serial_t *serial = (serial_t *)mmdvm->serial;
    if (serial_open(serial, 'x') != 0) {
        goto error;
    }
#define TRY_SERIAL(op, val) if (serial_##op(serial, val) != 0) goto error
    TRY_SERIAL(baudrate, mmdvm->baud);
    TRY_SERIAL(parity, SERIAL_PARITY_NONE);
    TRY_SERIAL(bits, 8);
    TRY_SERIAL(stopbits, 1);
    TRY_SERIAL(flowcontrol, SERIAL_FLOWCONTROL_NONE);
    TRY_SERIAL(xon_xoff, SERIAL_XON_XOFF_DISABLED);
#undef TRY_SERIAL

    return 0;

error:
    dmr_log_error("mmdvm: serial open %s failed: %s",
        serial_name(serial), strerror(errno));
    TALLOC_FREE(serial);
    TALLOC_FREE(mmdvm);
    return dmr_error(DMR_EINVAL);
}

DMR_API int dmr_mmdvm_parse_frame(dmr_mmdvm *mmdvm, dmr_mmdvm_frame frame, dmr_parsed_packet **parsed_out)
{
    if (mmdvm == NULL || frame == NULL) {
        return -1;
    }

    if (frame[0] != DMR_MMDVM_FRAME_START) {
        return -1;
    }

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
        return 0;

    case DMR_MMDVM_DMR_DATA1:
    case DMR_MMDVM_DMR_DATA2:
        if ((parsed = dmr_packet_decode(frame + 4)) == NULL) {
            return dmr_error(DMR_ENOMEM);
        }
        parsed->ts = (frame[3] & 0x80) >> 7;
        *parsed_out = parsed;
        return 0;

    default:
        dmr_log_warn("mmdvm: modem sent unknown command %#02x", frame[2]);
        return 0;
    }
}

DMR_API int dmr_mmdvm_read(void *mmdvmptr, int fd)
{
    DMR_UNUSED(fd);

    if (mmdvmptr == NULL)
        return dmr_error(DMR_EINVAL);

    dmr_mmdvm *mmdvm = (dmr_mmdvm *)mmdvmptr;
    serial_t *serial = (serial_t *)mmdvm->serial;

    /* Check if there is data available for us, if not, skip */
    switch (serial_read_waiting(serial)) {
    case -1:
        dmr_log_error("mmdvm: read ready(%s): %s",
            serial_name(serial), strerror(errno));
        return -1;
    case 0:
        return 0;
    default:
        break;
    }

    int ret;
    size_t len = DMR_MMDVM_FRAME_MAX - mmdvm->pos;
    if ((ret = serial_read(serial, mmdvm->frame + mmdvm->pos, len, 30)) == -1) {
        dmr_log_error("mmdvm: read(%s): %s", mmdvm->port, strerror(errno));
        return -1;
    }

    /* Shift the buffer until we have a FRAME_START byte */
    for (mmdvm->pos += ret; mmdvm->pos != 0; mmdvm->pos--) {
        if (mmdvm->frame[0] != DMR_MMDVM_FRAME_START) {
            memmove(mmdvm->frame, mmdvm->frame + 1, DMR_MMDVM_FRAME_MAX);
        }
    }

    /* If there is no FRAME_START byte, wait for the next read */
    if (mmdvm->frame[0] != DMR_MMDVM_FRAME_START) {
        dmr_log_debug("mmdvm: no FRAME START found, waiting for next read");
        return 0;
    }
    /* If the frame length doesn't match, wait for the next read */
    if (mmdvm->pos < 2) {
        dmr_log_debug("mmdvm: waiting for more data");
        return 0;
    }
    if (mmdvm->frame[1] < mmdvm->pos) {
        dmr_log_debug("mmdvm: not enough data received, at %u/%u", mmdvm->pos, mmdvm->frame[1]);
        return 0;
    }

    dmr_parsed_packet *parsed;
    ret = dmr_mmdvm_parse_frame(mmdvm, mmdvm->frame, &parsed);
    if (parsed != NULL) {
    }

    /* Shift the buffer */
    memmove(mmdvm->frame, mmdvm->frame + mmdvm->frame[1], DMR_MMDVM_FRAME_MAX);

    return ret;
}

DMR_API int dmr_mmdvm_write(void *mmdvmptr, int fd)
{
    DMR_UNUSED(fd);

    if (mmdvmptr == NULL)
        return dmr_error(DMR_EINVAL);

    dmr_mmdvm *mmdvm = (dmr_mmdvm *)mmdvmptr;
    if (STAILQ_EMPTY(&mmdvm->frameq))
        return 0;

    serial_t *serial = (serial_t *)mmdvm->serial;
    dmr_mmdvm_frameq *entry = STAILQ_FIRST(&mmdvm->frameq);
    int ret;
    do {
        ret = serial_write(serial, entry->frame, entry->frame[1], 30);
    } while (ret == -1 && (errno == EINVAL || errno == EAGAIN));
    if (ret == -1) {
        dmr_log_error("mmdvm: write(%s, %u) failed: %s",
            serial_name(serial), entry->frame[1], strerror(errno));
        return -1;
    }

    STAILQ_REMOVE(&mmdvm->frameq, entry, dmr_mmdvm_frameq, entries);
    TALLOC_FREE(entry);
    return 0;
}

DMR_PRV static dmr_mmdvm_frameq *new_frame(dmr_mmdvm *mmdvm, dmr_mmdvm_command command, uint8_t args, ...)
{
    dmr_mmdvm_frameq *entry = talloc_zero(mmdvm, dmr_mmdvm_frameq);
    if (entry == NULL)
        return NULL;

    int i;
    va_list ap;
 
    entry->frame[0] = DMR_MMDVM_FRAME_START;
    entry->frame[1] = args + 3;
    entry->frame[2] = command;

    va_start(ap, args);
    for (i = 0; i < args; i++) {
        entry->frame[i + 3] = (uint8_t)va_arg(ap, int);
    }
    va_end(ap);
    return entry;
}

DMR_API int dmr_mmdvm_set_config(dmr_mmdvm *mmdvm, uint8_t invert, uint8_t mode, uint8_t delay_ms, dmr_mmdvm_state state, uint8_t rx_level, uint8_t tx_level)
{
    if (mmdvm == NULL)
        return dmr_error(DMR_EINVAL);

    dmr_mmdvm_frameq *entry = new_frame(mmdvm, DMR_MMDVM_SET_CONFIG, 6,
        invert,
        mode,
        delay_ms,
        state,
        rx_level,
        tx_level);
    STAILQ_INSERT_TAIL(&mmdvm->frameq, entry, entries);

    return 0;
} 

int dmr_mmdvm_set_mode(dmr_mmdvm *mmdvm, uint8_t state)
{
    if (mmdvm == NULL)
        return dmr_error(DMR_EINVAL);

    dmr_mmdvm_frameq *entry = new_frame(mmdvm, DMR_MMDVM_SET_MODE, 1, state);
    STAILQ_INSERT_TAIL(&mmdvm->frameq, entry, entries);

    return 0;
}

DMR_API int dmr_mmdvm_set_rf_config(dmr_mmdvm *mmdvm, uint32_t rx_freq, uint32_t tx_freq)
{
    if (mmdvm == NULL)
        return dmr_error(DMR_EINVAL);

    if (mmdvm->model != DMR_MMDVM_MODEL_DVMEGA) {
        dmr_log_warn("mmdvm: set RF config not supported by model \"%s\"",
            dmr_mmdvm_model_name(mmdvm->model));
        return -1;
    }

    dmr_mmdvm_frameq *entry = new_frame(mmdvm, DMR_MMDVM_SET_CONFIG, 9,
        0x00, /* flags, reserved */
        (uint8_t)((rx_freq >> 24)),
        (uint8_t)((rx_freq >> 16)),
        (uint8_t)((rx_freq >>  8)),
        (uint8_t)((rx_freq >>  0)),
        (uint8_t)((tx_freq >> 24)),
        (uint8_t)((tx_freq >> 16)),
        (uint8_t)((tx_freq >>  8)),
        (uint8_t)((tx_freq >>  0)));
    STAILQ_INSERT_TAIL(&mmdvm->frameq, entry, entries);

    return 0;
}

DMR_API int dmr_mmdvm_close(void *mmdvmptr, int fd)
{
    DMR_UNUSED(fd);
    if (mmdvmptr == NULL)
        return dmr_error(DMR_EINVAL);

    dmr_mmdvm *mmdvm = (dmr_mmdvm *)mmdvmptr;
    serial_t *serial = (serial_t *)mmdvm->serial;
    if (serial != NULL) {
        serial_close(serial);
        serial_free(serial);
    }
    TALLOC_FREE(mmdvm);
    return 0;
}

DMR_API dmr_protocol dmr_mmdvm_protocol = {
    .type     = DMR_PROTOCOL_MMDVM,
    .name     = "Multi Mode Digital Voice Modem",
    .readable = dmr_mmdvm_read,
    .writable = dmr_mmdvm_write,
    .closable = dmr_mmdvm_close
};
