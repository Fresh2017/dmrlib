#include <unistd.h>
#include "common/socket.h"

ssize_t socket_write(socket_t *s, const void *buf, size_t len)
{
    if (s == NULL) {
        errno = EINVAL;
        return -1;
    }
    return write(s->fd, buf, len);
}
