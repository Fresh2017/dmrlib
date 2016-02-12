#ifndef _NOISEBRIDGE_CONFIG_H
#define _NOISEBRIDGE_CONFIG_H

#include <inttypes.h>
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <dmr/protocol.h>
#include <dmr/protocol/homebrew.h>
#if defined(WITH_MBELIB)
#include <dmr/proto/mbe.h>
#endif
#include <dmr/protocol/mmdvm.h>
#include "http.h"
#include "common/config.h"
#include "common/socket.h"

#define NOISEBRIDGE_MAX_PROTOS 16

typedef struct {
    dmr_protocol      protocol;
    dmr_protocol_type type;
    char              *name;
    void              *instance;
    int               fd;
    union {
        struct {
            //struct addrinfo *peer_addr;
            uint8_t         peer_ip[16];
            uint16_t        peer_port;
            //struct addrinfo *bind_addr;
            uint8_t         bind_ip[16];
            uint16_t        bind_port;
            char            *auth;
            char            *call;
            dmr_id          repeater_id;
            dmr_color_code  color_code;
            uint32_t        rx_freq;
            uint32_t        tx_freq;
            uint8_t         tx_power;
            int             height;
            float           latitude;
            float           longitude;
        } homebrew;
        struct {
            char *device;
            int  quality;
        } mbe;
        struct {
            char            *port;
            int             baud;
            dmr_mmdvm_model model;
            dmr_color_code  color_code;
            uint32_t        rx_freq;
            uint32_t        tx_freq;
        } mmdvm;
    } settings;
} proto_t;

typedef int (*parse_section_t)(char *line, char *filename, size_t lineno);

typedef struct {
    char            *filename;
    lua_State       *L;
    proto_t         *proto[NOISEBRIDGE_MAX_PROTOS];
    size_t          protos;
    parse_section_t section;
    struct {
        bool     enabled;
        ip6_t    bind;
        uint16_t port;
        char     *root;
    } httpd;
    struct {
        char     *script;
        uint16_t timeout;
    } repeater;
} config_t;

#if !defined(HAVE_GETLINE)
size_t getline(char **lineptr, size_t *n, FILE *stream);
#endif
#if !defined(HAVE_STRTOK_R)
char* strtok_r(char *str, const char *delim, char **nextp);
#endif

int init_config(char *filename);
config_t *load_config(void);

#endif // _NOISEBRIDGE_CONFIG_H
