#ifndef _DMR_PROTO_HOMEBREW_H
#define _DMR_PROTO_HOMEBREW_H

#include <netinet/in.h>

#include <dmr/type.h>
#include <dmr/packet.h>
#include <openssl/sha.h>

#define DMR_HOMEBREW_PORT               62030
#define DMR_HOMEBREW_DMR_DATA_SIZE      53

#define DMR_HOMEBREW_UNKNOWN_FRAME      0x00
#define DMR_HOMEBREW_DMR_DATA_FRAME     0x10
typedef uint8_t dmr_homebrew_frame_type_t;

#define DMR_HOMEBREW_DATA_TYPE_VOICE      0b00
#define DMR_HOMEBREW_DATA_TYPE_VOICE_SYNC 0b01
#define DMR_HOMEBREW_DATA_TYPE_DATA_SYNC  0b10
#define DMR_HOMEBREW_DATA_TYPE_INVALID    0b11

typedef enum {
    DMR_HOMEBREW_AUTH_NONE,
    DMR_HOMEBREW_AUTH_INIT,
    DMR_HOMEBREW_AUTH_DONE,
    DMR_HOMEBREW_AUTH_FAIL
} dmr_homebrew_auth_t;

typedef struct {
    char         signature[4];
    uint8_t      sequence;
    dmr_packet_t dmr_packet;
    uint8_t      call_type;
    uint8_t      frame_type;
    uint8_t      data_type;
    uint32_t     stream_id;
} dmr_homebrew_packet_t;

typedef struct {
    char    callsign[8];
    uint8_t repeater_id[4];
    uint8_t rx_freq[9];
    uint8_t tx_freq[9];
    uint8_t tx_power[2];
    uint8_t color_code[2];
    uint8_t latitude[8];
    uint8_t longitude[9];
    uint8_t height[3];
    char    location[20];
    char    description[20];
    char    url[124];
    char    software_id[40];
    char    package_id[40];
} dmr_homebrew_config_t;

typedef struct {
    int                   fd;
    struct sockaddr_in    server, remote;
    dmr_homebrew_auth_t   auth;
    dmr_id_t              id;
    uint8_t               buffer[64];
    uint8_t               random[8];
    dmr_homebrew_config_t *config;
    bool                  run;
} dmr_homebrew_t;

typedef struct __attribute__((__packed__)) {
    union {
        struct {
            uint8_t  signature[6];
            dmr_id_t repeater_id;
        } ack;
        struct {
            uint8_t  signature[6];
            dmr_id_t repeater_id;
        } nak;
    } response;
} dmr_homebrew_master_packet_t;

extern dmr_homebrew_t *dmr_homebrew_new(struct in_addr bind, int port, struct in_addr peer);
extern bool dmr_homebrew_auth(dmr_homebrew_t *homebrew, const char *secret);
extern void dmr_homebrew_loop(dmr_homebrew_t *homebrew);
extern bool dmr_homebrew_send(dmr_homebrew_t *homebrew, const uint8_t *buf, uint16_t len);
extern bool dmr_homebrew_recv(dmr_homebrew_t *homebrew, uint16_t *len);
extern dmr_homebrew_frame_type_t dmr_homebrew_frame_type(const uint8_t *bytes, unsigned int len);
extern dmr_homebrew_packet_t *dmr_homebrew_parse_packet(const uint8_t *bytes, unsigned int len);

extern dmr_homebrew_config_t *dmr_homebrew_config_new(void);
extern void dmr_homebrew_config_callsign(dmr_homebrew_config_t *config, const char *callsign);
extern void dmr_homebrew_config_repeater_id(dmr_homebrew_config_t *config, dmr_id_t repeater_id);
extern void dmr_homebrew_config_tx_power(dmr_homebrew_config_t *config, uint8_t tx_power);
extern void dmr_homebrew_config_color_code(dmr_homebrew_config_t *config, dmr_color_code_t color_code);
extern void dmr_homebrew_config_height(dmr_homebrew_config_t *config, int height);
extern void dmr_homebrew_config_location(dmr_homebrew_config_t *config, const char *location);
extern void dmr_homebrew_config_description(dmr_homebrew_config_t *config, const char *description);
extern void dmr_homebrew_config_url(dmr_homebrew_config_t *config, const char *url);
extern void dmr_homebrew_config_software_id(dmr_homebrew_config_t *config, const char *software_id);
extern void dmr_homebrew_config_package_id(dmr_homebrew_config_t *config, const char *package_id);

#endif // _DMR_PROTO_HOMEBREW_H
