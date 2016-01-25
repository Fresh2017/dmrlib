#include <errno.h>
#include <string.h>
#include "dmr/bits.h"
#include "dmr/log.h"
#include "dmr/serial.h"

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
     dmr_log_warn("mmdvm: serial_baud_lookup(): using non-standard baud rate: %ld", baud);
     return baud;
}

#if defined(DMR_PLATFORM_WINDOWS)
int dmr_serial_write(dmr_serial_t fd, void *buf, size_t len)
{
    dmr_log_trace("serial: write %d", len);
    if (dmr_log_priority() <= DMR_LOG_PRIORITY_DEBUG)
        dmr_dump_hex(buf, len);
    DWORD n;
    if (!WriteFile(fd, buf, len, &n, NULL))
        return -1;
    return (int)n;
}

int dmr_serial_read(dmr_serial_t fd, void *buf, size_t len)
{
    //dmr_log_trace("serial: read %d", len);
    DWORD n;
    if (!ReadFile(fd, buf, len, &n, NULL))
        return -1;
    return (int)n;
}

int dmr_serial_close(dmr_serial_t fd)
{
    if (!CloseHandle(fd))
        return -1;
    return 0;
}

int dmr_serial_error(void)
{
    return GetLastError();
}

const char *dmr_serial_error_message(void)
{
    LPVOID lpMsgBuf;
    DWORD dw = GetLastError();
    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        dw,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR) &lpMsgBuf,
        0, NULL );

    return lpMsgBuf;
}

#else

int dmr_serial_open(const char *port, long rate, bool rts)
{
    int fd;
    speed_t baud = serial_baud_lookup(rate);
    dmr_log_debug("serial: open %s with %ld baud and rts = %s", port, rate, DMR_LOG_BOOL(rts));
    if ((fd = open(port, O_RDWR | O_NOCTTY | O_NONBLOCK)) < 0) {
        dmr_log_error("serial: open %s failed: %s", port, dmr_serial_error_message());
        return -1;
    }
    if (!isatty(fd)) {
        dmr_log_error("serial: open %s failed: not a TTY\n", port);
        return -1;
    }
    if (fcntl(fd, F_SETFL, 0) != 0) {
        dmr_log_error("serial: fcntl %s failed: %s", port, strerror(errno));
        return -1;
    }

    struct termios settings;
    memset(&settings, 0, sizeof(struct termios));
    if (tcgetattr(fd, &settings) < 0) {
        dmr_log_error("serial: tcgetattr %s failed: %s", port, strerror(errno));
        return -1;
    }

    settings.c_lflag    &= ~(ECHO | ECHOE | ICANON | IEXTEN | ISIG);
    settings.c_iflag    &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON | IXOFF | IXANY);
    settings.c_cflag    &= ~(CSIZE | CSTOPB | PARENB | CRTSCTS);
    settings.c_cflag    |= CS8;
    settings.c_oflag    &= ~(OPOST);
    settings.c_cc[VMIN]  = 0;
    settings.c_cc[VTIME] = 10;
    cfsetispeed(&settings, baud);
    cfsetospeed(&settings, baud);
    if (tcsetattr(fd, TCSANOW, &settings) < 0) {
        dmr_log_error("serial: tcsetattr %s failed: %s", port, strerror(errno));
        return -1;
    }
    dmr_msleep(250);
    if (tcflush(fd, TCOFLUSH) != 0) {
        dmr_log_error("serial: tcoflush %s failed: %s", port, strerror(errno));
        return -1;
    }
    if (tcflush(fd, TCIFLUSH) != 0) {
        dmr_log_error("serial: tciflush %s failed: %s", port, strerror(errno));
        return -1;
    }

    if (rts) {
        dmr_log_trace("serial: enabling RTS");
        unsigned int r;
        if (ioctl(fd, TIOCMGET, &r) < 0) {
            dmr_log_error("serial: ioctl %s failed: %s", port, strerror(errno));
            return -1;
        }

        r |= TIOCM_RTS;
        if (ioctl(fd, TIOCMSET, &r) < 0) {
            dmr_log_error("serial: ioctl %s failed: %s", port, strerror(errno));
            return -1;
        }
    }

    return fd;
}

int dmr_serial_write(dmr_serial_t fd, void *buf, size_t len)
{
    dmr_log_trace("serial: write %d", len);
    if (dmr_log_priority() <= DMR_LOG_PRIORITY_DEBUG)
        dmr_dump_hex(buf, len);

    if (len == 0)
        return 0;

    size_t pos = 0;
    while (pos < len) {
        ssize_t n = write(fd, buf, len);
        if (n < 0 && errno != EAGAIN) {
            dmr_log_error("serial: write: %s", strerror(errno));
            return -1;
        }
        pos += n;
    }
    return pos;
}

int dmr_serial_read(dmr_serial_t fd, void *buf, size_t len, struct timeval *timeout)
{
    /*
    if (timeout == NULL) {
        dmr_log_trace("serial: read %d", len);
    } else {
        dmr_log_trace("serial: read %d in %ld.%06ld", len, timeout->tv_sec, timeout->tv_usec);
    }
    */

    if (len == 0)
        return 0;

    size_t pos = 0;
    while (pos < len) {
        int s;
        fd_set fds;
		FD_ZERO(&fds);
		FD_SET(fd, &fds);

        if (pos == 0) {
            struct timeval tv;
            if (timeout == NULL) {
                tv.tv_sec = 0;
                tv.tv_usec = 0;
            } else {
                tv.tv_sec = timeout->tv_sec;
                tv.tv_usec = timeout->tv_usec;
            }
            s = select(fd + 1, &fds, NULL, NULL, &tv);
        } else {
            s = select(fd + 1, &fds, NULL, NULL, NULL);
        }
        if (s == 0) {
            if (timeout != NULL) {
                // Timeouts can be expected here, log trace
                //dmr_log_trace("serial: select timeout");
                return 0;
            }
            dmr_log_error("serial: select timeout");
        } else if (s < 0) {
            dmr_log_error("serial: select: %s", strerror(errno));
            return -1;
        } else {
            ssize_t n = read(fd, buf + pos, len - pos);
            if (n < 0 && errno != EAGAIN) {
                dmr_log_error("serial: read: %s", strerror(errno));
                return -1;
            }
            if (n > 0)
                pos += n;
        }
    }
    return pos;
}

int dmr_serial_close(dmr_serial_t fd)
{
    dmr_log_trace("serial: close");
    return close(fd);
}

int dmr_serial_error(void)
{
    return errno;
}

const char *dmr_serial_error_message(void)
{
    return strerror(errno);
}

#endif // DMR_PLATFORM_WINDOWS
