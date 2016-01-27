#include <errno.h>
#include <sys/types.h>
#ifndef __MINGW32__
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#endif
#include "shared/config.h"
#include "shared/socket.h"
#include "shared/platform.h"

#ifndef EAFNOSUPPORT
#define EAFNOSUPPORT EINVAL
#endif
#ifndef EPFNOSUPPORT
#define EPFNOSUPPORT EAFNOSUPPORT
#endif
#ifndef EPROTONOSUPPORT
#define EPROTONOSUPPORT EAFNOSUPPORT
#endif

int socket_udp6(void)
{
#if defined(HAVE_LIBC_IPV6)
    int fd;
    __winsock_init();
    if (ip6disabled) {
        goto compat;
    }
    fd = __winsock_errno(socket(PF_INET6, SOCK_DGRAM, IPPROTO_UDP));
    if (fd == -1) {
        if (errno == EINVAL ||
            errno == EAFNOSUPPORT ||
            errno == EPFNOSUPPORT ||
            errno == EPROTONOSUPPORT) {
compat:
            fd = __winsock_errno(socket(AF_INET, SOCK_DGRAM, 0));
            ip6disabled = true;
            if (fd == -1) {
                return fd;
            }
        }
        return fd;
    }
#if defined(IPV6_V6ONLY)
    int zero = 0;
    __winsock_errno(setsockopt(fd, IPPROTO_IPV6, IPV6_V6ONLY, (void *)&zero, sizeof(zero)));
#endif
    if (socket_nodelay(fd) == -1) {
        close(fd);
        return -1;
    }
    return fd;
#else // HAVE_LIBC_IPV6
    return -1;
#endif
}
