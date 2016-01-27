#include <sys/types.h>
#include <sys/param.h>
#if !defined(__MINGW32__)
#include <sys/socket.h>
#include <netinet/in.h>
#endif
#include "shared/socket.h"
#include "shared/platform.h"
#include "shared/uint.h"

ssize_t socket_recv4(int fd, uint8_t *buf, size_t len, ip4_t ip, uint16_t *port)
{
    struct sockaddr_in si;
    socklen_t silen = sizeof(si);
    ssize_t r;

    if ((r = recvfrom(fd, buf, len, 0, (struct sockaddr *)&si, &silen)) < 0) {
        return __winsock_errno(-1);
    }
    if (ip != NULL) {
        *(uint32_t *)ip = *(uint32_t *)&si.sin_addr;
    }
    if (port != NULL) {
        *port = uint16((uint8_t *)&si.sin_port);
    }
    return r;
}
