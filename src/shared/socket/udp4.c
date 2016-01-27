#include <sys/types.h>
#ifndef __MINGW32__
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#endif
#include "shared/socket.h"
#include "shared/platform.h"

int socket_udp4(void)
{
    int fd;
    __winsock_init();
    fd = __winsock_errno(socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP));
    if (fd == -1) {
        return fd;
    }
    if (socket_nodelay(fd) == -1) {
        close(fd);
        return -1;
    }
    return fd;
}
