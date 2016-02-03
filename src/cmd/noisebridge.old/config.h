#ifndef _NOISEBRIDGE_CONFIG_H
#define _NOISEBRIDGE_CONFIG_H

#include <dmr/log.h>
#include <dmr/proto.h>
#include <dmr/proto/homebrew.h>
#include <dmr/proto/mbe.h>
#include <dmr/proto/mmdvm.h>
#include <dmr/proto/repeater.h>
#include <dmr/proto/sms.h>
#include <dmr/payload/voice.h>

#include "route.h"
#include "module.h"

extern const char *software_id, *package_id;

typedef enum {
    PEER_NONE,
    PEER_HOMEBREW,
    PEER_MBE,
    PEER_MMDVM
} peer_t;

#define NOISEBRIDGE_MAX_PLUGINS 64

typedef struct {
    const char *filename;
    const char **argv;
    int        argc;
    module_t   *module;
#if defined(HAVE_DL)
    void       *handle;
#endif
} plugin_t;

typedef struct config_s {
    const char            *filename;
    peer_t                upstream, modem;
    route_rule_t          *repeater_route[ROUTE_RULE_MAX];
    size_t                repeater_routes;
    dmr_color_code_t      repeater_color_code;
    char                  *http_addr;
    int                   http_port;
    char                  *http_root;
    dmr_homebrew_t        *homebrew;
    dmr_homebrew_config_t homebrew_config;
    dmr_id_t              homebrew_id;
    char                  *homebrew_host_s;
    struct hostent        *homebrew_host;
    int                   homebrew_port;
    char                  *homebrew_auth;
    dmr_mmdvm_t           *mmdvm;
    char                  *mmdvm_port;
    int                   mmdvm_rate;
    uint32_t              mmdvm_rx_freq;
    uint32_t              mmdvm_tx_freq;
    dmr_mbe_t             *mbe;
    uint8_t               mbe_quality;
    dmr_sms_t             *sms;
    char                  *sms_path;
    bool                  audio_needed;
    char                  *audio_device;
    dmr_log_priority_t    log_level;
    plugin_t              *plugin[NOISEBRIDGE_MAX_PLUGINS];
    size_t                plugins;
} config_t;

char *join_path(char *dir, char *filename);
config_t *load_config(void);
config_t *init_config(const char *filename);
void kill_config();

#endif // _NOISEBRIDGE_CONFIG_H
