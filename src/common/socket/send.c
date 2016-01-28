#include <errno.h>
#include <sys/types.h>
#include <sys/param.h>
#if !defined(__MINGW32__)
#include <sys/socket.h>
#include <netinet/in.h>
#endif
#include "common/config.h"
#include "common/byte.h"
#include "common/platform.h"
#include "common/socket.h"
#include "common/uint.h"

ssize_t socket_send(socket_t *s, uint8_t *buf, size_t len, ip6_t ip, uint16_t port)
{
    if (s == NULL) {
        errno = EINVAL;
        return -1;
    }
    return s->v == 4
        ? socket_send4(s->fd, buf, len, ip, port)
        : socket_send6(s->fd, buf, len, ip, port, s->scope_id);
}

ssize_t socket_send4(int fd, uint8_t *buf, size_t len, ip4_t ip, uint16_t port)
{
    struct sockaddr_in si;
    byte_zero(&si, sizeof(si));
    si.sin_family = AF_INET;
    uint16_pack((uint8_t *) &si.sin_port, port);
    *((uint32_t *)&si.sin_addr) = *((uint32_t *)ip);
    return __winsock_errno(sendto(fd, buf, len, 0, (void *)&si, sizeof(si)));
}

ssize_t socket_send6(int fd, uint8_t *buf, size_t len, ip6_t ip, uint16_t port, uint32_t scope_id)
{
#if defined(HAVE_LIBC_IPV6)
    struct sockaddr_in6 si;
#else
    struct sockaddr_in si;
    (void)scope_id;
#endif
    byte_zero(&si, sizeof(si));
#if defined(HAVE_LIBC_IPV6)
    if (ip6disabled) {
#endif
        if (isip4mapped(ip)) {
            return socket_send4(fd, buf, len, ip + 12, port);
        }
        if (byte_equal((void *)ip, (void *)ip6loopback, 16)) {
            return socket_send4(fd, buf, len, (uint8_t *)ip4loopback, port);
        }
#if defined(HAVE_LIBC_IPV6)
        errno = EPROTONOSUPPORT;
        return -1;
    }
    si.sin6_family = AF_INET;
    uint16_pack((uint8_t *)&si.sin6_port, port);
    byte_copy((uint8_t *)&si.sin6_addr, ip, 16);
#if defined(HAVE_LIBC_SCOPE_ID)
    si.sin6_scope_id = scope_id;
#else
    si.sin6_scope_id = 0;
#endif
    return __winsock_errno(sendto(fd, buf, len, 0, (void *)&si, sizeof(si)));
#else
    errno = EPROTONOSUPPORT;
    return -1;
#endif
}
