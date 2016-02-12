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

    dmr_log_debug("mmdvm io: init io");

    dmr_mmdvm *mmdvm = (dmr_mmdvm *)mmdvmptr;

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
        dmr_log_info("mmdvm io: tuning to %u/%u Hz", mmdvm->rx_freq, mmdvm->tx_freq);
        ret = dmr_mmdvm_set_rf_config(mmdvm, mmdvm->rx_freq, mmdvm->tx_freq);
    }

    return ret;
}

DMR_PRV static int mmdvm_io_register(dmr_io *io, void *mmdvmptr)
{
    DMR_ERROR_IF_NULL(io, DMR_EINVAL);
    DMR_ERROR_IF_NULL(mmdvmptr, DMR_EINVAL);

    dmr_log_debug("mmdvm io: register io");

    dmr_mmdvm *mmdvm = (dmr_mmdvm *)mmdvmptr;
    serial_t *serial = (serial_t *)mmdvm->serial;
    int fd = serial->fd;
    
    /* register events */
    dmr_io_reg_read (io, fd, mmdvm_io_readable, mmdvm, false);
    dmr_io_reg_write(io, fd, mmdvm_io_writable, mmdvm, false);
    dmr_io_reg_error(io, fd, mmdvm_io_error,    mmdvm, false);

    if (mmdvm->model == DMR_MMDVM_MODEL_G4KLX) {
        struct timeval status_timer = { 5, 0 };
        dmr_io_reg_timer(io, status_timer, mmdvm_io_status_timer, mmdvm, false);
    }

    return 0;
}

DMR_PRV static int mmdvm_io_status_timer(dmr_io *io, void *mmdvmptr)
{
    DMR_ERROR_IF_NULL(io, DMR_EINVAL);
    DMR_ERROR_IF_NULL(mmdvmptr, DMR_EINVAL);

    dmr_log_trace("mmdvm io: status timer");

    dmr_mmdvm *mmdvm = (dmr_mmdvm *)mmdvmptr;

    return dmr_mmdvm_get_status(mmdvm);
}

DMR_PRV static int mmdvm_io_readable(dmr_io *io, void *mmdvmptr, int fd)
{
    DMR_UNUSED(fd);
    DMR_ERROR_IF_NULL(io, DMR_EINVAL);
    DMR_ERROR_IF_NULL(mmdvmptr, DMR_EINVAL);

    dmr_log_trace("mmdvm io: readable");

    dmr_mmdvm *mmdvm = (dmr_mmdvm *)mmdvmptr;
    dmr_parsed_packet **parsed;
    DMR_ERROR_IF_NULL(parsed = dmr_malloc(dmr_parsed_packet *), DMR_ENOMEM);

    int ret = dmr_mmdvm_read(mmdvm, parsed);
    if (ret == 0 && *parsed != NULL) {
        dmr_log_debug("mmdvm io: queued parsed packet");
        ret = dmr_packetq_add(mmdvm->rxq, *parsed);
        dmr_free(*parsed);
    } else {
        dmr_log_debug("mmdvm io: no parsed packet");
    }

    dmr_free(parsed);
    return ret;
}

DMR_PRV static int mmdvm_io_writable(dmr_io *io, void *mmdvmptr, int fd)
{
    DMR_UNUSED(fd);
    DMR_ERROR_IF_NULL(io, DMR_EINVAL);
    DMR_ERROR_IF_NULL(mmdvmptr, DMR_EINVAL);

    dmr_log_trace("mmdvm io: writable");

    dmr_mmdvm *mmdvm = (dmr_mmdvm *)mmdvmptr;

    return dmr_mmdvm_write(mmdvm);
}

DMR_PRV static int mmdvm_io_error(dmr_io *io, void *mmdvmptr, int fd)
{
    DMR_UNUSED(fd);
    DMR_ERROR_IF_NULL(io, DMR_EINVAL);
    DMR_ERROR_IF_NULL(mmdvmptr, DMR_EINVAL);

    dmr_log_trace("mmdvm io: error");

    dmr_mmdvm *mmdvm = (dmr_mmdvm *)mmdvmptr;
    dmr_log_critical("mmdvm io: serial error");
    return dmr_mmdvm_close(mmdvm);
}

DMR_API dmr_protocol dmr_mmdvm_protocol = {
    .type        = DMR_PROTOCOL_MMDVM,
    .name        = "Multi Mode Digital Voice Modem",
    .init_io     = mmdvm_io_init,
    .register_io = mmdvm_io_register
};
