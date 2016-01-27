#include <sys/types.h>
#include <sys/param.h>
#if !defined(__MINGW32__)
#include <sys/socket.h>
#include <netinet/in.h>
#endif
#include "shared/config.h"
#include "shared/socket.h"
#include "shared/platform.h"
#include "shared/uint.h"

ssize_t socket_recv6(int fd, uint8_t *buf, size_t len, ip6_t ip, uint16_t *port, uint32_t *scope_id)
{
#if defined(HAVE_LIBC_IPV6)
    struct sockaddr_in6 si;
#else
    struct sockaddr_in si;
#endif
    socklen_t silen = sizeof(si);
    ssize_t r;
    byte_zero(&si, silen);
    if ((r = recvfrom(fd, buf, len, 0, (struct sockaddr *)&si, &silen)) < 0) {
        return __winsock_errno(-1);
    }

#if defined(HAVE_LIBC_IPV6)
    if (ip6disabled) {
        struct sockaddr_in *si4 = (struct sockaddr_in *)&si;
        if (ip != NULL) {
            byte_copy(ip, (uint8_t *)ip6mappedv4prefix, 12);
            byte_copy(ip + 12, &si4->sin_addr, 4);
        }
        if (port != NULL) {
            *port = uint16((uint8_t *)&si4->sin_port);
        }
        return r;
    }
    if (ip != NULL) {
        byte_copy(ip, (uint8_t *)&si.sin6_addr, 16);
    }
    if (port != NULL) {
        *port = uint16((uint8_t *)&si.sin6_port);
    }
#if defined(HAVE_LIBC_SCOPE_ID)
    if (scope_id != NULL) {
        *scope_id = si.sin6_scope_id;
    }
#else
    *scope_id = 0;
#endif
#else // HAVE_LIBC_IPV6
    if (ip != NULL) {
        byte_copy(ip, (uint8_t *)ip6mappedv4prefix, 12);
        byte_copy(ip + 12, &si.sin_addr, 4);
    }
    if (port != NULL) {
        *port = uint16((uint8_t *)&si.sin_addr);
    }
    if (scope_id != NULL) {
        *scope_id = 0;
    }
#endif
    return r;
}
