#ifndef _COMMON_SOCKET_H
#define _COMMON_SOCKET_H

#include <errno.h>
#include <inttypes.h>
#include <sys/types.h>
#include <stdbool.h>
#include "common/config.h"
#include "common/byte.h"
#include "common/platform.h"
#if defined(PLATFORM_WINDOWS)
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

#if defined(__cplusplus)
extern "C" {
#endif

#if !defined(EAFNOSUPPORT)
#define EAFNOSUPPORT EINVAL
#endif

#if !defined(EPFNOSUPPORT)
#define EPFNOSUPPORT EAFNOSUPPORT
#endif

#if !defined(EPROTONOSUPPORT)
#define EPROTONOSUPPORT EAFNOSUPPORT
#endif

#if !defined(HAVE_SOCKLEN_T)
typedef int socklen_t;
#endif

typedef struct {
    int      fd;
    uint8_t  v;
    uint32_t scope_id;
} socket_t;

typedef uint8_t ip4_t[4];
typedef uint8_t ip6_t[16];

extern const ip4_t   ip4loopback;
extern const ip4_t   ip4any;
extern const uint8_t ip6mappedv4prefix[12];
extern const ip6_t   ip6loopback;
extern const ip6_t   ip6any;
extern bool          ip6disabled;

/* Wrapper functions */
socket_t   *socket_new(uint8_t v, uint32_t scope_id);
#define     socket_new4()           socket_new(4, 0)
#define     socket_new6(scope_id)   socket_new(6, scope_id)
void        socket_free(socket_t *s);
#define     socket_free4 socket_free
#define     socket_free6 socket_free
socket_t   *socket_tcp4(void);
socket_t   *socket_tcp6(uint32_t scope_id);
socket_t   *socket_udp4(void);
socket_t   *socket_udp6(uint32_t scope_id);

int         socket_bind(socket_t *s, const ip6_t ip, uint16_t port);
int         socket_connect(socket_t *s, const ip6_t ip, uint16_t port);
bool        socket_connected(socket_t *s);
int         socket_listen(socket_t *s, unsigned int backlog);
ssize_t     socket_recv(socket_t *s, uint8_t *buf, size_t len, ip6_t ip, uint16_t *port);
ssize_t     socket_send(socket_t *s, uint8_t *buf, size_t len, ip6_t ip, uint16_t port);

/* Low level functions */
int         socket_bind4(int fd, const ip6_t ip, uint16_t port);
int         socket_bind6(int fd, const ip6_t ip, uint16_t port, uint32_t scope_id);
int         socket_connect4(int fd, const ip4_t ip, uint16_t port);
int         socket_connect6(int fd, const ip6_t ip, uint16_t port, uint32_t scope_id);
bool        socket_connectedx(int fd);
#define     socket_connected4 socket_connectedx
#define     socket_connected6 socket_connectedx
uint32_t    socket_getifindex(const char *ifname);
const char *socket_getifname(uint32_t _interface);
int         socket_listenx(int fd, unsigned int backlog);
#define     socket_listen4 socket_listenx
#define     socket_listen6 socket_listenx
int         socket_nodelay(int fd);
int         socket_nodelay_off(int fd);
ssize_t     socket_recv4(int fd, uint8_t *buf, size_t len, ip4_t ip, uint16_t *port);
ssize_t     socket_recv6(int fd, uint8_t *buf, size_t len, ip6_t ip, uint16_t *port, uint32_t *scope_id);
ssize_t     socket_send4(int fd, uint8_t *buf, size_t len, ip4_t ip, uint16_t port);
ssize_t     socket_send6(int fd, uint8_t *buf, size_t len, ip6_t ip, uint16_t port, uint32_t scope_id);

#define isip4mapped(ip) (byte_equal((uint8_t *)(ip), (uint8_t *)ip6mappedv4prefix, 12))

#if defined(__cplusplus)
}
#endif

#endif // _COMMON_SOCKET_H
