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

ssize_t socket_send4(int fd, uint8_t *buf, size_t len, ip4_t ip, uint16_t port)
{
    struct sockaddr_in si;
    byte_zero(&si, sizeof(si));
    si.sin_family = AF_INET;
    uint16_pack((uint8_t *) &si.sin_port, port);
    *((uint32_t *)&si.sin_addr) = *((uint32_t *)ip);
    return __winsock_errno(sendto(fd, buf, len, 0, (void *)&si, sizeof(si)));
}
