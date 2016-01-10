#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <ctype.h>

#include "dmr/platform.h"

#if defined(DMR_PLATFORM_WINDOWS)
#include <windows.h>
#include <assert.h>
#else
#include <fcntl.h>
#include <termios.h>
#endif

#include "dmr/error.h"
#include "dmr/log.h"
#include "dmr/proto.h"
#include "dmr/proto/mmdvm.h"
#include "dmr/thread.h"
#include "dmr/serial.h"

static const char *dmr_mmdvm_proto_name = "mmdvm";

struct baud_mapping {
  long    baud;
  speed_t speed;
};

static struct baud_mapping baud_lookup_table[] = {
#if defined(DMR_PLATFORM_WINDOWS)
    { 1200,   CBR_1200 },
    { 2400,   CBR_2400 },
    { 4800,   CBR_4800 },
    { 9600,   CBR_9600 },
    { 19200,  CBR_19200 },
    { 38400,  CBR_38400 },
    { 57600,  CBR_57600 },
    { 115200, CBR_115200 },
#else
    { 1200,   B1200 },
    { 2400,   B2400 },
    { 4800,   B4800 },
    { 9600,   B9600 },
    { 19200,  B19200 },
    { 38400,  B38400 },
#ifdef B57600
    { 57600,  B57600 },
#endif
#ifdef B115200
    { 115200, B115200 },
#endif
#ifdef B230400
    { 230400, B230400 },
#endif
#endif
    { 0,      0 }                 /* Terminator. */
};

static speed_t serial_baud_lookup(long baud)
{
    struct baud_mapping *map = baud_lookup_table;

    while (map->baud) {
        if (map->baud == baud)
            return map->speed;
        map++;
    }

    /*
     * If a non-standard BAUD rate is used, issue
     * a warning (if we are verbose) and return the raw rate
     */
     fprintf(stderr, "mmdvm: serial_baud_lookup(): using non-standard baud rate: %ld", baud);
     return baud;
}


static dmr_proto_status_t mmdvm_proto_init(void *modemptr)
{
    dmr_log_verbose("mmdvm: init");
    dmr_mmdvm_t *modem = (dmr_mmdvm_t *)modemptr;
    if (modem == NULL)
        return DMR_PROTO_CONF;

    return dmr_mmdvm_sync(modem) ? DMR_PROTO_OK : DMR_PROTO_NOT_READY;
}

static int mmdvm_proto_start_thread(void *modemptr)
{
    dmr_log_verbose("mmdvm: start thread");
    dmr_mmdvm_t *modem = (dmr_mmdvm_t *)modemptr;
    if (modem == NULL) {
        fprintf(stderr, "mmdvm: start thread called without modem?!\n");
        return dmr_thread_error;
    }

    while (modem->active) {
        dmr_mmdvm_poll(modem);
        dmr_msleep(250);
    }

    return dmr_thread_success;
}

static bool mmdvm_proto_start(void *modemptr)
{
    dmr_log_verbose("mmdvm: start");
    int ret;
    dmr_mmdvm_t *modem = (dmr_mmdvm_t *)modemptr;
    if (modem == NULL)
        return false;

    if (modem->thread != NULL) {
        fprintf(stderr, "mmdvm: can't start, already active\n");
        return false;
    }

    modem->thread = malloc(sizeof(dmr_thread_t));
    if (modem->thread == NULL) {
        fprintf(stderr, "mmdvm: can't start, out of memory\n");
        return false;
    }

    switch ((ret = dmr_thread_create(modem->thread, mmdvm_proto_start_thread, modemptr))) {
    case dmr_thread_success:
        break;
    default:
        fprintf(stderr, "mmdvm: can't create thread\n");
        return false;
    }

    return true;
}

static bool mmdvm_proto_stop(void *modemptr)
{
    dmr_log_verbose("mmdvm: stop");
    int ret;
    dmr_mmdvm_t *modem = (dmr_mmdvm_t *)modemptr;
    if (modem == NULL)
        return false;

    if (modem->thread == NULL) {
        fprintf(stderr, "mmdvm: not active\n");
        return true;
    }

    modem->active = false;
    switch ((ret = dmr_thread_join(*modem->thread, NULL))) {
    case dmr_thread_success:
        break;
    default:
        fprintf(stderr, "mmdvm: can't stop thread\n");
        return false;
    }

    free(modem->thread);
    modem->thread = NULL;

    dmr_mmdvm_close(modem);
    return true;
}

static bool mmdvm_proto_active(void *modemptr)
{
    dmr_log_verbose("mmdvm: active?");
    dmr_mmdvm_t *modem = (dmr_mmdvm_t *)modemptr;
    if (modem == NULL)
        return false;

    return modem->thread != NULL && modem->active;
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

dmr_mmdvm_t *dmr_mmdvm_open(char *port, long baud, size_t buffer_sizes)
{
    dmr_mmdvm_t *modem = malloc(sizeof(dmr_mmdvm_t));
    if (modem == NULL)
        return NULL;

    memset(modem, 0, sizeof(dmr_mmdvm_t));

    // Setup MMDVM protocol
    modem->proto.name = dmr_mmdvm_proto_name;
    modem->proto.init = mmdvm_proto_init;
    modem->proto.start = mmdvm_proto_start;
    modem->proto.stop = mmdvm_proto_stop;
    modem->proto.active = mmdvm_proto_active;

    // Setup MMDVM modem
    modem->version = 0;
    modem->firmware = NULL;
    modem->baud = serial_baud_lookup(baud);
#ifdef DMR_PLATFORM_WINDOWS
    char portbuf[64];
    memset(portbuf, 0, 64);
    snprintf(portbuf, 64, "\\\\.\\%s", port);
    modem->port = portbuf;
#else
modem->port = strdup(port);
#endif
    modem->dstar_rx_buffer = dmr_ring_new(buffer_sizes);
    modem->dstar_tx_buffer = dmr_ring_new(buffer_sizes);
    modem->dmr_ts1_rx_buffer = dmr_ring_new(buffer_sizes);
    modem->dmr_ts1_tx_buffer = dmr_ring_new(buffer_sizes);
    modem->dmr_ts2_rx_buffer = dmr_ring_new(buffer_sizes);
    modem->dmr_ts2_tx_buffer = dmr_ring_new(buffer_sizes);
    modem->ysf_rx_buffer = dmr_ring_new(buffer_sizes);
    modem->ysf_tx_buffer = dmr_ring_new(buffer_sizes);
    if (modem->port              == NULL ||
        modem->dstar_rx_buffer   == NULL ||
        modem->dstar_tx_buffer   == NULL ||
        modem->dmr_ts1_rx_buffer == NULL ||
        modem->dmr_ts1_tx_buffer == NULL ||
        modem->dmr_ts2_rx_buffer == NULL ||
        modem->dmr_ts2_tx_buffer == NULL ||
        modem->ysf_rx_buffer     == NULL ||
        modem->ysf_tx_buffer     == NULL) {
        dmr_mmdvm_free(modem);
        return NULL;
    }

#if defined(DMR_PLATFORM_WINDOWS)
    modem->fd = CreateFile(modem->port, GENERIC_READ|GENERIC_WRITE, 0, NULL,
        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (modem->fd == INVALID_HANDLE_VALUE) {
        modem->error = dmr_serial_error();
        dmr_log_error("mmdvm: open %s failed: %s", modem->port, dmr_serial_error_message());
        return modem;
    }

    DCB settings = {0};
    settings.DCBlength = sizeof(settings);
    if (GetCommState(modem->fd, &settings) == 0) {
        modem->error = dmr_serial_error();
        dmr_log_error("mmdvm: error getting %s device state: %s", modem->port, dmr_serial_error_message());
        return modem;
    }

    settings.BaudRate = modem->baud;
    settings.ByteSize = 8;
    settings.StopBits = ONESTOPBIT;
    settings.Parity = NOPARITY;
    if (SetCommState(modem->fd, &settings) == 0) {
        modem->error = dmr_serial_error();
        dmr_log_error("mmdvm: error setting %s device state: %s", modem->port, dmr_serial_error_message());
        return modem;
    }

    COMMTIMEOUTS timeouts = {0};
    timeouts.ReadIntervalTimeout = 50;
    timeouts.ReadTotalTimeoutConstant = 50;
    timeouts.ReadTotalTimeoutMultiplier = 10;
    timeouts.WriteTotalTimeoutConstant = 50;
    timeouts.WriteTotalTimeoutMultiplier = 10;
    if (SetCommTimeouts(modem->fd, &timeouts) == 0) {
        modem->error = dmr_serial_error();
        dmr_log_error("mmdvm: error setting %s timeouts: %s", modem->port, dmr_serial_error_message());
        return modem;
    }

#else
    if ((modem->fd = open(modem->port, O_RDWR | O_NOCTTY | O_NONBLOCK)) < 0) {
        modem->error = errno;
        dmr_log_error("mmdvm: open %s failed: %s", modem->port, dmr_serial_error_message());
        return modem;
    }
    if (!isatty(modem->fd)) {
        fprintf(stderr, "mmdvm open %s failed: not a TTY\n", modem->port);
        return modem;
    }

    struct termios settings;
    memset(&settings, 0, sizeof(struct termios));
    if ((modem->error = tcgetattr(modem->fd, &settings)) != 0) {
        fprintf(stderr, "mmdvm tcgetattr failed: %s\n", strerror(modem->error));
        return modem;
    }
    /*
    settings.c_lflag &= ~(ECHO | ECHOE | ICANON | IEXTEN | ISIG);
	settings.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON | IXOFF | IXANY);
	settings.c_cflag &= ~(CSIZE | CSTOPB | PARENB | CRTSCTS);
	settings.c_cflag |= CS8;
	settings.c_oflag &= ~(OPOST);
	settings.c_cc[VMIN] = 1;
	settings.c_cc[VTIME] = 10;
    */
    // 8N1
    /*
    settings.c_cflag &= ~PARENB;
    settings.c_cflag &= ~CSTOPB;
    settings.c_cflag &= ~CSIZE;
    settings.c_cflag |= CS8;
    // no flow control
    settings.c_cflag &= ~CRTSCTS;
    //toptions.c_cflag &= ~HUPCL; // disable hang-up-on-close to avoid reset
    settings.c_cflag |= CREAD | CLOCAL;  // turn on READ & ignore ctrl lines
    settings.c_iflag &= ~(IXON | IXOFF | IXANY); // turn off s/w flow ctrl
    settings.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG); // make raw
    settings.c_oflag &= ~OPOST; // make raw
    settings.c_cc[VMIN] = 1;
    settings.c_cc[VTIME] = 2;
    */
    // set new port settings for non-canonical input processing  //must be NOCTTY
    settings.c_lflag    &= ~(ECHO | ECHOE | ICANON | IEXTEN | ISIG);
    settings.c_iflag    &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON | IXOFF | IXANY);
    settings.c_cflag    &= ~(CSIZE | CSTOPB | PARENB | CRTSCTS);
    settings.c_cflag    |= CS8;
    settings.c_oflag    &= ~(OPOST);
    settings.c_cc[VMIN]  = 0;
    settings.c_cc[VTIME] = 10;
    cfsetispeed(&settings, modem->baud);
    cfsetospeed(&settings, modem->baud);
    if ((modem->error = tcsetattr(modem->fd, TCSANOW, &settings)) != 0) {
        fprintf(stderr, "mmdvm tcsetattr failed: %s\n", strerror(modem->error));
        return modem;
    }
    dmr_msleep(250);
    if ((modem->error = tcflush(modem->fd, TCOFLUSH)) != 0) {
        fprintf(stderr, "mmdvm tcflush failed: %s\n", strerror(modem->error));
        return modem;
    }
    if ((modem->error = tcflush(modem->fd, TCIFLUSH)) != 0) {
        fprintf(stderr, "mmdvm tcflush failed: %s\n", strerror(modem->error));
        return modem;
    }
#endif

    return modem;
}

int dmr_mmdvm_poll(dmr_mmdvm_t *modem)
{
    if (modem == NULL || modem->fd == 0)
        return dmr_error(DMR_EINVAL);

    uint8_t len, length, data;
    uint16_t val[4];
    dmr_mmdvm_response_t res = dmr_mmdvm_get_response(modem, &len);

    switch (res) {
    case dmr_mmdvm_timeout:
        break; // Nothing to do here

    case dmr_mmdvm_error:
        fprintf(stderr, "mmdvm read error\n");
        break;

    case dmr_mmdvm_ok:
        length = modem->buffer[1];
        switch (modem->buffer[2]) {
        case DMR_MMDVM_DMR_DATA1:
        case DMR_MMDVM_DMR_DATA2:
            dmr_log_debug("mmdvm: DMR data on TS%d, slot type %s (%d)",
                modem->buffer[2] - DMR_MMDVM_DMR_DATA1,
                dmr_packet_get_slot_type_name(modem->buffer[3]),
                modem->buffer[3]);
            dump_hex(modem->buffer, len);

            dmr_packet_t *packet = dmr_packet_decode(&modem->buffer[4], len - 4);
            if (packet == NULL) {
                dmr_log_error("mmdvm: unable to decode DMR packet");
                return -1;
            }
            packet->ts = modem->buffer[2] - DMR_MMDVM_DMR_DATA1;
            if (mmdvm_proto_active(modem)) {
                dmr_log_debug("mmdvm: proto active, relaying packet");
                dmr_proto_rx(&modem->proto, modem, packet);
            }
            free(packet);
            break;

        case DMR_MMDVM_DMR_LOST1:
            dmr_log_debug("mmdvm: DMR lost TS1");
            data = 1;
            dmr_ring_write(modem->dmr_ts1_rx_buffer, &data, 1);
            data = DMR_MMDVM_TAG_LOST;
            dmr_ring_write(modem->dmr_ts1_rx_buffer, &data, 1);
            break;

        case DMR_MMDVM_DMR_LOST2:
            dmr_log_debug("mmdvm: DMR lost TS2");
            data = 1;
            dmr_ring_write(modem->dmr_ts2_rx_buffer, &data, 1);
            data = DMR_MMDVM_TAG_LOST;
            dmr_ring_write(modem->dmr_ts2_rx_buffer, &data, 1);
            break;

        case DMR_MMDVM_GET_STATUS:
            dmr_log_debug("mmdvm: get status response");
            modem->rx = (modem->buffer[5] & 0x01) == 0x01;
            modem->adc_overflow = (modem->buffer[5] & 0x02) == 0x02;
            modem->space.dstar = modem->buffer[6];
            modem->space.dmr_ts1 = modem->buffer[7];
            modem->space.dmr_ts2 = modem->buffer[8];
            modem->space.ysf = modem->buffer[9];
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
                fprintf(stderr, "mmdvm: debug1 %.*s\n",
                    length - 3, &modem->buffer[3]);
                break;
            case DMR_MMDVM_DEBUG2:
                val[0] = modem->buffer[length - 2] << 8 | modem->buffer[length - 1];
                fprintf(stderr, "mmdvm: debug2 %.*s %d\n",
                    length - 5, &modem->buffer[3], val[0]);
                break;
            case DMR_MMDVM_DEBUG3:
                val[0] = modem->buffer[length - 4] << 8 | modem->buffer[length - 3];
                val[1] = modem->buffer[length - 2] << 8 | modem->buffer[length - 1];
                fprintf(stderr, "mmdvm: debug3 %.*s %d %d\n",
                    length - 7, &modem->buffer[3], val[0], val[1]);
                break;
            case DMR_MMDVM_DEBUG4:
                val[0] = modem->buffer[length - 6] << 8 | modem->buffer[length - 5];
                val[1] = modem->buffer[length - 4] << 8 | modem->buffer[length - 3];
                val[2] = modem->buffer[length - 2] << 8 | modem->buffer[length - 1];
                fprintf(stderr, "mmdvm: debug4 %.*s %d %d %d\n",
                    length - 9, &modem->buffer[3], val[0], val[1], val[2]);
                break;
            case DMR_MMDVM_DEBUG5:
                val[0] = modem->buffer[length - 8] << 8 | modem->buffer[length - 7];
                val[1] = modem->buffer[length - 6] << 8 | modem->buffer[length - 5];
                val[2] = modem->buffer[length - 4] << 8 | modem->buffer[length - 3];
                val[3] = modem->buffer[length - 2] << 8 | modem->buffer[length - 1];
                fprintf(stderr, "mmdvm: debug5 %.*s %d %d %d %d\n",
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
            dump_hex(&modem->buffer[3], length - 2);
            break;
        }
    }

    return 0;
}

int dmr_mmdvm_sync(dmr_mmdvm_t *modem)
{
    uint8_t i;

    if (modem == NULL)
        return dmr_error(DMR_EINVAL);

    dmr_log_debug("mmdvm: sync modem on %s rate %u", modem->port, modem->baud);
    dmr_msleep(DMR_MMDVM_DELAY_MS);
    for (i = 0; i < 6; i++) {
        uint8_t buf[3] = {DMR_MMDVM_FRAME_START, 3, DMR_MMDVM_GET_VERSION};

        dmr_log_debug("mmdvm write: DMR_MMDVM_FRAME_START, 3, DMR_MMDVM_GET_VERSION");
        if (dmr_serial_write(modem->fd, &buf, 3) != 3) {
            modem->error = errno;
            return dmr_error(DMR_EWRITE);
        }

        uint8_t len;
        dmr_mmdvm_response_t res = dmr_mmdvm_get_response(modem, &len);
        dmr_log_debug("mmdvm: %d bytes of response", len);
        dump_hex(modem->buffer, len);
        if (res == dmr_mmdvm_ok && modem->buffer[2] == DMR_MMDVM_GET_VERSION) {
            dmr_log_info("mmdvm: protocol %d, %.*s",
                modem->buffer[3], len - 4, &modem->buffer[4]);

            modem->version = modem->buffer[3];
            modem->firmware = malloc(len - 3);
            memset(modem->firmware, 0, len - 3);
            memcpy(modem->firmware, &modem->buffer[4], len - 4);
            return 0;
        }

        dmr_sleep(1);
    }

    dmr_log_error("mmdvm: sync error: unable to read firmware version");
    return -1;
}

dmr_mmdvm_response_t dmr_mmdvm_get_response(dmr_mmdvm_t *modem, uint8_t *length)
{
    int ret = 0;
    uint8_t offset = 0;
    uint8_t *p = &modem->buffer[0];
#ifdef DMR_PLATFORM_WINDOWS
    OVERLAPPED ov;
    DWORD dwEventMask;
#else
    struct timeval timeout;
    fd_set rfds;
    int nfds;
#endif

    if (modem == NULL)
        return dmr_mmdvm_error;

    // Clear buffer
    memset(modem->buffer, 0, sizeof(modem->buffer));
    *length = 0;

#ifndef DMR_PLATFORM_WINDOWS
    timeout.tv_sec = 0;
    timeout.tv_usec = 250000;
#endif
    while (true) {
#ifdef DMR_PLATFORM_WINDOWS
        memset(&ov, 0, sizeof(ov));
        ov.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
        assert(ov.hEvent);
        BOOL abRet = WaitCommEvent(modem->fd, &dwEventMask, &ov);
        if (!abRet) {
            DWORD dwRet = GetLastError();
            if (dwRet != ERROR_IO_PENDING) {
                dmr_log_error("mmdvm: read failed: %s", dmr_serial_error_message());
                return dmr_mmdvm_error;
            }
        }

#else
select_again:
        FD_ZERO(&rfds);
        FD_SET(modem->fd, &rfds);
        nfds = select(modem->fd + 1, &rfds, NULL, NULL, NULL);
        if (nfds == 0) {
            fprintf(stderr, "mmdvm: read failed: modem not responding\n");
            return dmr_mmdvm_timeout;
        } else if (nfds == -1) {
            if (errno == EINTR || errno == EAGAIN) {
                fprintf(stderr, "mmdvm: read failed: modem not responding, retrying\n");
                goto select_again;
            }
            modem->error = errno;
            fprintf(stderr, "mmdvm: read failed: select(): %s\n", strerror(modem->error));
            return dmr_mmdvm_error;
        }
#endif // DMR_PLATFORM_WINDOWS

        // If we have no length, read frame start and length only
        if (*length == 0) {
            ret = dmr_serial_read(modem->fd, p, 2);
            dmr_log_debug("mmdvm: read %d/2 bytes", ret);
        } else {
            ret = dmr_serial_read(modem->fd, p, *length - offset);
            dmr_log_debug("mmdvm: read %d/%d bytes, length = %d", ret, *length - offset, *length);
        }
        if (ret <= 0) {
            modem->error = errno;
            dmr_log_error("mmdvm read failed: read(): %s\n", strerror(modem->error));
            return dmr_mmdvm_error;
        }

        offset += ret;
        p += ret;

        if (*length == 0 && offset >= 2) {
            fprintf(stderr, "mmdvm read: 0x%02x\n", modem->buffer[0]);
            if (modem->buffer[0] != DMR_MMDVM_FRAME_START) {
                fprintf(stderr, "mmdvm read failed: expected FRAME_START, got 0x%02x\n", modem->buffer[0]);
                return dmr_mmdvm_timeout;
            }
            if (modem->buffer[1] > 200) {
                fprintf(stderr, "mmdvm read failed: length overflow\n");
                return dmr_mmdvm_error;
            }
            *length = modem->buffer[1];
        }

        if (*length > 0 && offset == *length)
            break;
    }

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

    if (modem->port != NULL)
        free(modem->port);
    if (modem->dstar_rx_buffer != NULL)
        free(modem->dstar_rx_buffer);
    if (modem->dstar_tx_buffer != NULL)
        free(modem->dstar_tx_buffer);
    if (modem->dmr_ts1_rx_buffer != NULL)
        free(modem->dmr_ts1_rx_buffer);
    if (modem->dmr_ts1_tx_buffer != NULL)
        free(modem->dmr_ts1_tx_buffer);
    if (modem->dmr_ts2_rx_buffer != NULL)
        free(modem->dmr_ts2_rx_buffer);
    if (modem->dmr_ts2_tx_buffer != NULL)
        free(modem->dmr_ts2_tx_buffer);
    if (modem->ysf_rx_buffer != NULL)
        free(modem->ysf_rx_buffer);
    if (modem->ysf_tx_buffer != NULL)
        free(modem->ysf_tx_buffer);

    free(modem);
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
