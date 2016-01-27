#include <errno.h>
#include <sys/types.h>
#include <sys/param.h>
#if !defined(__MINGW32__)
#include <sys/socket.h>
#include <netinet/in.h>
#endif
#include "shared/config.h"
#include "shared/byte.h"
#include "shared/platform.h"
#include "shared/socket.h"
#include "shared/uint.h"

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
