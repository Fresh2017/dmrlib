#include <errno.h>
#if defined(__MINGW32__)
#include <winsock2.h>
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#endif
#include "common/config.h"
#include "common/platform.h"
#include "common/byte.h"
#include "common/uint.h"
#include "common/socket.h"

int socket_bind(socket_t *s, const ip6_t ip, uint16_t port)
{
    if (s == NULL) {
        errno = EINVAL;
        return -1;
    }
    return s->v == 4
        ? socket_bind4(s->fd, ip, port)
        : socket_bind6(s->fd, ip, port, s->scope_id);
}

int socket_bind4(int fd, const ip4_t ip, uint16_t port)
{
    struct sockaddr_in si;
    byte_zero(&si, sizeof(si));
    si.sin_family = AF_INET;
    uint16_pack((uint8_t *)&si.sin_port, port);
    if (ip != NULL) {
        *(uint32_t *)&si.sin_addr = *(uint32_t *)ip;
    } else {
        si.sin_addr.s_addr = INADDR_ANY;
    }
    return __winsock_errno(bind(fd, (struct sockaddr*)&si, sizeof(si)));
}

int socket_bind6(int fd, const ip6_t ip, uint16_t port, uint32_t scope_id)
{
#if defined(HAVE_LIBC_IPV6)
    struct sockaddr_in6 sa;
#endif
    int i;
#if !defined(HAVE_LIBC_SCOPE_ID)
    (void)scope_id;
#endif

    if (ip == NULL) {
        ip = ip6any;
    }

#if defined(HAVE_LIBC_IPV6)
    if (ip6disabled) {
#endif
        for (i = 0; i < 16; i++) {
            if (ip[i] != 0) break;
        }
        if (i == 16 || isip4mapped(ip)) {
            return socket_bind4(fd, ip + 12, port);
        }
#if defined(HAVE_LIBC_IPV6)
    }
    byte_zero(&sa, sizeof(sa));
    sa.sin6_family = AF_INET6;
    sa.sin6_flowinfo = 0;
#if defined(HAVE_LIBC_SCOPE_ID)
    sa.sin6_scope_id = scope_id;
#endif
    uint16_pack((uint8_t *)&sa.sin6_port, port);
    byte_copy((uint8_t *)&sa.sin6_addr, (void *)ip, 16);
    return __winsock_errno(bind(fd, (struct sockaddr *)&sa, sizeof(sa)));
#else
    errno = EPROTONOSUPPORT;
    return -1;
#endif
}
