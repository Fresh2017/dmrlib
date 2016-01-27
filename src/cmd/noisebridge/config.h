#ifndef _NOISEBRIDGE_CONFIG_H
#define _NOISEBRIDGE_CONFIG_H

#include <inttypes.h>
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <shared/config.h>
#include <dmr/proto.h>
#include <dmr/proto/homebrew.h>
#if defined(WITH_MBELIB)
#include <dmr/proto/mbe.h>
#endif
#include <dmr/proto/mmdvm.h>
#include <dmr/proto/repeater.h>

#define NOISEBRIDGE_MAX_PROTOS 16

typedef struct {
    dmr_proto_t      *proto;
    dmr_proto_type_t type;
    char             *name;
    void             *mem;
    union {
        struct {
            struct in_addr   *addr;
            int              port;
            char             *auth;
            char             *call;
            dmr_id_t         repeater_id;
            dmr_color_code_t color_code;
            uint32_t         rx_freq;
            uint32_t         tx_freq;
            uint8_t          tx_power;
            int              height;
            float            latitude;
            float            longitude;
        } homebrew;
        struct {
            char *device;
            int  quality;
        } mbe;
        struct {
            char              *port;
            dmr_mmdvm_model_t model;
        } mmdvm;
    } instance;
} proto_t;

typedef int (*parse_section_t)(char *line, char *filename, size_t lineno);

typedef struct {
    char            *filename;
    char            *script;
    lua_State       *L;
    proto_t         *proto[NOISEBRIDGE_MAX_PROTOS];
    size_t          protos;
    parse_section_t section;
} config_t;

int init_config(char *filename);
config_t *load_config(void);

#endif // _NOISEBRIDGE_CONFIG_H
