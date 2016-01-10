#include <errno.h>
#include <string.h>
#include "dmr/serial.h"

#if defined(DMR_PLATFORM_WINDOWS)
int dmr_serial_write(dmr_serial_t fd, void *buf, size_t len)
{
    DWORD n;
    if (!WriteFile(fd, buf, len, &n, NULL))
        return -1;
    return (int)n;
}

int dmr_serial_read(dmr_serial_t fd, void *buf, size_t len)
{
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

int dmr_serial_write(dmr_serial_t fd, void *buf, size_t len)
{
    return write(fd, buf, len);
}

int dmr_serial_read(dmr_serial_t fd, void *buf, size_t len)
{
    return read(fd, buf, len);
}

int dmr_serial_close(dmr_serial_t fd)
{
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
