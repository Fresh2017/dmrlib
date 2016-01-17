#ifndef _DMR_SERIAL_H
#define _DMR_SERIAL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <dmr/platform.h>

#if defined(DMR_PLATFORM_WINDOWS)
#include <windows.h>
#include <stdio.h>
typedef HANDLE dmr_serial_t;
typedef PDWORD dmr_serial_size_t;

#else
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>
typedef int dmr_serial_t;
typedef int dmr_serial_size_t;

#endif // DMR_PLATFORM_WINDOWS

extern int dmr_serial_open(const char *port, long rate, bool rts);
extern int dmr_serial_write(dmr_serial_t fd, void *buf, size_t len);
extern int dmr_serial_read(dmr_serial_t fd, void *buf, size_t len, struct timeval *timeout);
extern int dmr_serial_close(dmr_serial_t fd);
extern int dmr_serial_error(void);
extern const char *dmr_serial_error_message(void);

#ifdef __cplusplus
}
#endif

#endif // _DMR_SERIAL_H
