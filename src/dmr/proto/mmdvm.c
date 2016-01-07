#include <errno.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <ctype.h>

#include "dmr/proto/mmdvm.h"

struct baud_mapping {
  long    baud;
  speed_t speed;
};

static struct baud_mapping baud_lookup_table[] = {
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

#define HEXDUMP_COLS 16
static void dump_hex(void *mem, unsigned int len)
{
    unsigned int i, j;
    for(i = 0; i < len + ((len % HEXDUMP_COLS) ? (HEXDUMP_COLS - len % HEXDUMP_COLS) : 0); i++) {
        /* print offset */
        if (i % HEXDUMP_COLS == 0) {
            printf("0x%06x: ", i);
        }

        /* print hex data */
        if (i < len) {
            printf("%02x ", 0xFF & ((char*)mem)[i]);
        } else { /* end of block, just aligning for ASCII dump */
            printf("   ");
        }

        /* print ASCII dump */
        if (i % HEXDUMP_COLS == (HEXDUMP_COLS - 1)) {
            for (j = i - (HEXDUMP_COLS - 1); j <= i; j++) {
                if (j >= len) { /* end of block, not really printing */
                    putchar(' ');
                } else if (isprint(((char*)mem)[j])) { /* printable char */
                    putchar(0xff & ((char*)mem)[j]);
                } else { /* other char */
                    putchar('.');
                }
            }
            putchar('\n');
        }
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

dmr_mmdvm_t *dmr_mmdvm_open(char *port, long baud, size_t buffer_sizes)
{
    dmr_mmdvm_t *modem = malloc(sizeof(dmr_mmdvm_t));
    if (modem == NULL)
        return NULL;

    memset(modem, 0, sizeof(dmr_mmdvm_t));
    modem->version = 0;
    modem->firmware = NULL;
    modem->baud = serial_baud_lookup(baud);
    modem->port = strdup(port);
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

    if ((modem->fd = open(modem->port, O_RDWR | O_NOCTTY | O_NONBLOCK)) < 0) {
        modem->error = errno;
        fprintf(stderr, "mmdvm open %s failed: %s\n", modem->port, strerror(modem->error));
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
    cfsetispeed(&settings, modem->baud);
    cfsetospeed(&settings, modem->baud);
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
    settings.c_cc[VTIME] = 0;
    if ((modem->error = tcsetattr(modem->fd, TCSANOW, &settings)) != 0) {
        fprintf(stderr, "mmdvm tcsetattr failed: %s\n", strerror(modem->error));
        return modem;
    }
    usleep(250000);
    if ((modem->error = tcflush(modem->fd, TCOFLUSH)) != 0) {
        fprintf(stderr, "mmdvm tcflush failed: %s\n", strerror(modem->error));
        return modem;
    }
    if ((modem->error = tcflush(modem->fd, TCIFLUSH)) != 0) {
        fprintf(stderr, "mmdvm tcflush failed: %s\n", strerror(modem->error));
        return modem;
    }

    return modem;
}

void dmr_mmdvm_poll(dmr_mmdvm_t *modem)
{
    if (modem == NULL || modem->fd == 0)
        return;

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
            data = len - 2;
            dmr_ring_write(modem->dmr_ts1_rx_buffer, &data, 1);
            if (modem->buffer[3] == (DMR_SLOT_TYPE_SYNC_DATA | DMR_SLOT_TYPE_VOICE_LC))
                data = DMR_MMDVM_TAG_EOT;
            else
                data = DMR_MMDVM_TAG_DATA;
            dmr_ring_write(modem->dmr_ts1_rx_buffer, &data, 1);
            dmr_ring_write(modem->dmr_ts1_rx_buffer, &modem->buffer[3], len - 3);
            break;

        case DMR_MMDVM_DMR_DATA2:
            data = len - 2;
            dmr_ring_write(modem->dmr_ts2_rx_buffer, &data, 1);
            if (modem->buffer[3] == (DMR_SLOT_TYPE_SYNC_DATA | DMR_SLOT_TYPE_VOICE_LC))
                data = DMR_MMDVM_TAG_EOT;
            else
                data = DMR_MMDVM_TAG_DATA;
            dmr_ring_write(modem->dmr_ts2_rx_buffer, &data, 1);
            dmr_ring_write(modem->dmr_ts2_rx_buffer, &modem->buffer[3], len - 3);
            break;

        case DMR_MMDVM_DMR_LOST1:
            data = 1;
            dmr_ring_write(modem->dmr_ts1_rx_buffer, &data, 1);
            data = DMR_MMDVM_TAG_LOST;
            dmr_ring_write(modem->dmr_ts1_rx_buffer, &data, 1);
            break;

        case DMR_MMDVM_DMR_LOST2:
            data = 1;
            dmr_ring_write(modem->dmr_ts2_rx_buffer, &data, 1);
            data = DMR_MMDVM_TAG_LOST;
            dmr_ring_write(modem->dmr_ts2_rx_buffer, &data, 1);
            break;

        case DMR_MMDVM_GET_STATUS:
            modem->rx = (modem->buffer[5] & 0x01) == 0x01;
            modem->adc_overflow = (modem->buffer[5] & 0x02) == 0x02;
            modem->space.dstar = modem->buffer[6];
            modem->space.dmr_ts1 = modem->buffer[7];
            modem->space.dmr_ts2 = modem->buffer[8];
            modem->space.ysf = modem->buffer[9];
            break;

        case DMR_MMDVM_GET_VERSION:
		case DMR_MMDVM_ACK:
			break;

        case DMR_MMDVM_NAK:
            fprintf(stderr, "mmdvm received NAK, command %s (0x%02x), reason %u\n",
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
                fprintf(stderr, "mmdvm debug: %.*s\n",
                    length - 3, &modem->buffer[3]);
                break;
            case DMR_MMDVM_DEBUG2:
                val[0] = modem->buffer[length - 2] << 8 | modem->buffer[length - 1];
                fprintf(stderr, "mmdvm debug: %.*s %d\n",
                    length - 5, &modem->buffer[3], val[0]);
                break;
            case DMR_MMDVM_DEBUG3:
                val[0] = modem->buffer[length - 4] << 8 | modem->buffer[length - 3];
                val[1] = modem->buffer[length - 2] << 8 | modem->buffer[length - 1];
                fprintf(stderr, "mmdvm debug: %.*s %d %d\n",
                    length - 7, &modem->buffer[3], val[0], val[1]);
                break;
            case DMR_MMDVM_DEBUG4:
                val[0] = modem->buffer[length - 6] << 8 | modem->buffer[length - 5];
                val[1] = modem->buffer[length - 4] << 8 | modem->buffer[length - 3];
                val[2] = modem->buffer[length - 2] << 8 | modem->buffer[length - 1];
                fprintf(stderr, "mmdvm debug: %.*s %d %d %d\n",
                    length - 9, &modem->buffer[3], val[0], val[1], val[2]);
                break;
            case DMR_MMDVM_DEBUG5:
                val[0] = modem->buffer[length - 8] << 8 | modem->buffer[length - 7];
                val[1] = modem->buffer[length - 6] << 8 | modem->buffer[length - 5];
                val[2] = modem->buffer[length - 4] << 8 | modem->buffer[length - 3];
                val[3] = modem->buffer[length - 2] << 8 | modem->buffer[length - 1];
                fprintf(stderr, "mmdvm debug: %.*s %d %d %d %d\n",
                    length - 11, &modem->buffer[3], val[0], val[1], val[2], val[3]);
                break;
            }
            break;

        case DMR_MMDVM_SAMPLES:
            break; // FIXME(maze): not implemented

        default:
            fprintf(stderr, "mmdvm sent unhandled response %d (%s), length %d\n",
                modem->buffer[2], dmr_mmdvm_command_name(modem->buffer[2]),
                modem->buffer[1]);
            dump_hex(&modem->buffer[3], length - 2);
            break;
        }
    }
}

bool dmr_mmdvm_sync(dmr_mmdvm_t *modem)
{
    int ret;

    if (modem == NULL)
        return false;

    usleep(DMR_MMDVM_DELAY_US);

    for (uint8_t i = 0; i < 6; i++) {
        uint8_t buf[3] = {DMR_MMDVM_FRAME_START, 3, DMR_MMDVM_GET_VERSION};

        fprintf(stderr, "mmdvm write: DMR_MMDVM_FRAME_START, 3, DMR_MMDVM_GET_VERSION\n");
        if ((ret = write(modem->fd, &buf, 3)) != 3) {
            modem->error = errno;
            fprintf(stderr, "mmdvm write failed: %s\n", strerror(modem->error));
            return false;
        }

        uint8_t len;
        dmr_mmdvm_response_t res = dmr_mmdvm_get_response(modem, &len);
        printf("got %d bytes of response\n", len);
        dump_hex(modem->buffer, len);
        if (res == dmr_mmdvm_ok && modem->buffer[2] == DMR_MMDVM_GET_VERSION) {
            fprintf(stderr, "mmdvm modem protocol version %d, software: %.*s\n",
                modem->buffer[3], len - 4, &modem->buffer[4]);

            modem->version = modem->buffer[3];
            modem->firmware = malloc(len - 3);
            memset(modem->firmware, 0, len - 3);
            memcpy(modem->firmware, &modem->buffer[4], len - 4);
            return true;
        }

        sleep(1);
    }

    fprintf(stderr, "mmdvm sync error: unable to read firmware version\n");
    return false;
}

dmr_mmdvm_response_t dmr_mmdvm_get_response(dmr_mmdvm_t *modem, uint8_t *length)
{
    int ret, retry = 0;
    struct timeval timeout;
    uint8_t offset = 0;
    fd_set rfds;
    int nfds;
    uint8_t *p = &modem->buffer[0];

    if (modem == NULL)
        return dmr_mmdvm_error;

    // Clear buffer
    memset(modem->buffer, 0, 200);
    *length = 0;

    timeout.tv_sec = 0;
    timeout.tv_usec = 250000;
    while (true) {
    select_again:
        FD_ZERO(&rfds);
        FD_SET(modem->fd, &rfds);

        nfds = select(modem->fd + 1, &rfds, NULL, NULL, &timeout);
        if (nfds == 0) {
            fprintf(stderr, "mmdvm read failed: modem not responding\n");
            return dmr_mmdvm_timeout;
        } else if (nfds == -1) {
            if (errno == EINTR || errno == EAGAIN) {
                fprintf(stderr, "mmdvm read failed: modem not responding, retrying\n");
                goto select_again;
            }
            modem->error = errno;
            fprintf(stderr, "mmdvm read failed: select(): %s\n", strerror(modem->error));
            return dmr_mmdvm_error;
        }

        // If we have no length, read frame start and length only
        if (*length == 0) {
            ret = read(modem->fd, p, 2);
        } else {
            ret = read(modem->fd, p, *length - offset);
        }
        if (ret < 0) {
            modem->error = errno;
            fprintf(stderr, "mmdvm read failed: read(): %s\n", strerror(modem->error));
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
    return write(modem->fd, &buffer, 3) == 3;
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

    return write(modem->fd, &buffer, 10) == 10;
}

bool dmr_mmdvm_set_mode(dmr_mmdvm_t *modem, uint8_t mode)
{
    if (modem == NULL || modem->fd == 0)
        return false;

    uint8_t buffer[4] = {DMR_MMDVM_FRAME_START, 4, DMR_MMDVM_SET_MODE, mode};
    return write(modem->fd, &buffer, 4) == 4;
}

bool dmr_mmdvm_dmr_start(dmr_mmdvm_t *modem, bool tx)
{
    if (modem == NULL || modem->fd == 0)
        return false;

    uint8_t buffer[4] = {DMR_MMDVM_FRAME_START, 4, DMR_MMDVM_DMR_START, tx ? 1 : 0};
    return write(modem->fd, &buffer, 4) == 4;
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
    return write(modem->fd, &buffer, 12) == 12;
}

void dmr_mmdvm_free(dmr_mmdvm_t *modem)
{
    if (modem == NULL)
        return;

    if (modem->fd > 0)
        dmr_mmdvm_close(modem);

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
}

void dmr_mmdvm_close(dmr_mmdvm_t *modem)
{
    if (modem == NULL)
        return;
    if (modem->fd == 0)
        return;

    modem->error = close(modem->fd);
    if (modem->error != 0) {
        fprintf(stderr, "mmdvm close failed: %s\n", strerror(modem->error));
    }
    modem->fd = 0;
}
