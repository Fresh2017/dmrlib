#ifndef _SHARED_SOCKET_H
#define _SHARED_SOCKET_H

#include <inttypes.h>
#include <sys/types.h>
#include "shared/byte.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int     fd;
    uint8_t v;
} socket_t;

typedef uint8_t ip4_t[4];
typedef uint8_t ip6_t[16];

extern const ip4_t   ip4loopback;
extern const ip4_t   ip4any;
extern const uint8_t ip6mappedv4prefix[12];
extern const ip6_t   ip6loopback;
extern const ip6_t   ip6any;
extern bool          ip6disabled;

int socket_listen(int fd, unsigned int backlog);
int socket_nodelay(int fd);
int socket_nodelay_off(int fd);
ssize_t socket_send4(int fd, uint8_t *buf, size_t len, ip4_t ip, uint16_t port);
ssize_t socket_send6(int fd, uint8_t *buf, size_t len, ip6_t ip, uint16_t port, uint32_t scope_id);
int socket_udp4(void);
int socket_udp6(void);
int socket_tcp4(void);
int socket_udp6(void);

#define isip4mapped(ip) (byte_equal((uint8_t *)(ip), (uint8_t *)ip6mappedv4prefix, 12))

#ifdef __cplusplus
}
#endif

#endif // _SHARED_SOCKET_H
