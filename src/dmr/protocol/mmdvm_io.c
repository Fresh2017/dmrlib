#include "dmr/error.h"
#include "dmr/malloc.h"

#include "dmr/log.h"
#include "dmr/protocol.h"
#include "dmr/protocol/mmdvm.h"
#include "common/byte.h"
#include "common/serial.h"

DMR_PRV static int mmdvm_io_status_timer(dmr_io *io, void *mmdvmptr);
DMR_PRV static int mmdvm_io_readable(dmr_io *io, void *mmdvmptr, int fd);
DMR_PRV static int mmdvm_io_writable(dmr_io *io, void *mmdvmptr, int fd);
DMR_PRV static int mmdvm_io_error(dmr_io *io, void *mmdvmptr, int fd);

DMR_PRV static int mmdvm_io_init(dmr_io *io, void *mmdvmptr)
{
    DMR_ERROR_IF_NULL(io, DMR_EINVAL);
    DMR_ERROR_IF_NULL(mmdvmptr, DMR_EINVAL);

    dmr_mmdvm *mmdvm = (dmr_mmdvm *)mmdvmptr;

    DMR_MM_DEBUG("io: init io");

    /* Setup queues */
    if (mmdvm->rxq == NULL)
        DMR_ERROR_IF_NULL(mmdvm->rxq = dmr_packetq_new(), DMR_ENOMEM);
    if (mmdvm->txq == NULL)
        DMR_ERROR_IF_NULL(mmdvm->txq = dmr_packetq_new(), DMR_ENOMEM);

    int ret = 0;
    if (!mmdvm->started) {
        ret = dmr_mmdvm_start(mmdvm);
    }
    if (ret == 0 && (mmdvm->rx_freq != 0 || mmdvm->tx_freq != 0)) {
        if (!mmdvm->ack[DMR_MMDVM_SET_RF_CONFIG]) {
            DMR_MM_DEBUG("io: no set RF config ack, retry");
#if defined(DMR_DEBUG)
            dmr_dump_hex((void *)mmdvm->ack, sizeof(mmdvm->ack));
#endif
            DMR_MM_INFO("io: tuning to %u/%u Hz", mmdvm->rx_freq, mmdvm->tx_freq);
            ret = dmr_mmdvm_set_rf_config(mmdvm, mmdvm->rx_freq, mmdvm->tx_freq);
        }
    }

    return ret;
}

DMR_PRV static int mmdvm_io_register(dmr_io *io, void *mmdvmptr)
{
    DMR_ERROR_IF_NULL(io, DMR_EINVAL);
    DMR_ERROR_IF_NULL(mmdvmptr, DMR_EINVAL);
    
    dmr_mmdvm *mmdvm = (dmr_mmdvm *)mmdvmptr;

    DMR_MM_DEBUG("io: register io");

    serial_t *serial = (serial_t *)mmdvm->serial;

    /* poll the modem status every second */
    struct timeval status_timer = { 1, 0 };
    
    /* register events */
    dmr_io_reg_timer(io, status_timer, mmdvm_io_status_timer, mmdvm, false);
    dmr_io_reg_read (io, serial->fd,   mmdvm_io_readable,     mmdvm, false);
    dmr_io_reg_write(io, serial->fd,   mmdvm_io_writable,     mmdvm, false);
    dmr_io_reg_error(io, serial->fd,   mmdvm_io_error,        mmdvm, false);

    return 0;
}

DMR_PRV static int mmdvm_io_status_timer(dmr_io *io, void *mmdvmptr)
{
    DMR_ERROR_IF_NULL(io, DMR_EINVAL);
    DMR_ERROR_IF_NULL(mmdvmptr, DMR_EINVAL);

    dmr_mmdvm *mmdvm = (dmr_mmdvm *)mmdvmptr;

    DMR_MM_TRACE("io: status timer");

    int ret = dmr_mmdvm_get_status(mmdvm);
    if (ret == 0 && (mmdvm->rx_freq != 0 || mmdvm->tx_freq != 0)) {
        if (!mmdvm->ack[DMR_MMDVM_SET_RF_CONFIG]) {
            DMR_MM_DEBUG("io: no set RF config ack, retry");
#if defined(DMR_DEBUG)
            dmr_dump_hex((void *)mmdvm->ack, sizeof(mmdvm->ack));
#endif
            DMR_MM_INFO("io: tuning to %u/%u Hz", mmdvm->rx_freq, mmdvm->tx_freq);
            ret = dmr_mmdvm_set_rf_config(mmdvm, mmdvm->rx_freq, mmdvm->tx_freq);
        }
    }
    return ret;
}

DMR_PRV static int mmdvm_io_readable(dmr_io *io, void *mmdvmptr, int fd)
{
    DMR_UNUSED(fd);
    DMR_ERROR_IF_NULL(io, DMR_EINVAL);
    DMR_ERROR_IF_NULL(mmdvmptr, DMR_EINVAL);
    
    dmr_mmdvm *mmdvm = (dmr_mmdvm *)mmdvmptr;

    DMR_MM_TRACE("io: readable");

    dmr_parsed_packet **parsed;
    DMR_ERROR_IF_NULL(parsed = dmr_malloc(dmr_parsed_packet *), DMR_ENOMEM);

    int ret = dmr_mmdvm_read(mmdvm, parsed);
    if (ret == 0 && *parsed != NULL) {
        DMR_MM_DEBUG("io: queued parsed packet");
        ret = dmr_packetq_add(mmdvm->rxq, *parsed);
        dmr_free(*parsed);
    } else {
        DMR_MM_DEBUG("io: no parsed packet");
    }

    dmr_free(parsed);
    return ret;
}

DMR_PRV static int mmdvm_io_writable(dmr_io *io, void *mmdvmptr, int fd)
{
    DMR_UNUSED(fd);
    DMR_ERROR_IF_NULL(io, DMR_EINVAL);
    DMR_ERROR_IF_NULL(mmdvmptr, DMR_EINVAL);

    dmr_mmdvm *mmdvm = (dmr_mmdvm *)mmdvmptr;

    DMR_MM_TRACE("io: writable");

    return dmr_mmdvm_write(mmdvm);
}

DMR_PRV static int mmdvm_io_error(dmr_io *io, void *mmdvmptr, int fd)
{
    DMR_UNUSED(fd);
    DMR_ERROR_IF_NULL(io, DMR_EINVAL);
    DMR_ERROR_IF_NULL(mmdvmptr, DMR_EINVAL);
    
    dmr_mmdvm *mmdvm = (dmr_mmdvm *)mmdvmptr;

    DMR_MM_TRACE("io: error");

    DMR_MM_FATAL("io: serial error");
    return dmr_mmdvm_close(mmdvm);
}

DMR_API dmr_protocol dmr_mmdvm_protocol = {
    .type        = DMR_PROTOCOL_MMDVM,
    .name        = "Multi Mode Digital Voice Modem",
    .init_io     = mmdvm_io_init,
    .register_io = mmdvm_io_register
};
