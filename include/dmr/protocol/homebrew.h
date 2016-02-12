/**
 * @file   Homebrew IPSC protocol (Brandmeister)
 * @brief  This implements the Homebrew IP Site Connect protocol as offered by
 *         DMR repeaters running the Brandmeister software stack.
 * @author Wijnand Modderman-Lenstra PD0MZ
 */
#ifndef _DMR_PROTOCOL_HOMEBREW_H
#define _DMR_PROTOCOL_HOMEBREW_H

#include <inttypes.h>
#include <netdb.h>
#include <dmr/config.h>
#include <dmr/packet.h>
#include <dmr/packetq.h>
#include <dmr/raw.h>
#include <dmr/time.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DMR_HOMEBREW_PORT 62030

typedef enum {
    DMR_HOMEBREW_AUTH_NONE = 0,
    DMR_HOMEBREW_AUTH_INIT,
    DMR_HOMEBREW_AUTH_CONFIG,
    DMR_HOMEBREW_AUTH_DONE,
    DMR_HOMEBREW_AUTH_FAILED
} dmr_homebrew_state;

typedef struct {
    char *id; /* identificaiton string */
    struct {
        char           *call;
        dmr_id         repeater_id;
        uint16_t       rx_freq;
        uint16_t       tx_freq;
        uint8_t        tx_power;
        dmr_color_code color_code;
        double         latitude;
        double         longitude;
        uint16_t       altitude;
        char           *location;
        char           *description;
        char           *url;
        char           *software_id;
        char           *package_id;
    } config;
    /* private */
    void           *sock;
    uint8_t        peer_ip[16];
    uint16_t       peer_port;
    uint8_t        bind_ip[16];
    uint16_t       bind_port;
    uint8_t        state;
    char           *secret;         /* secret set by dmr_homebrew_auth */
    uint8_t        nonce[8];        /* nonce sent by repeater */
    dmr_packetq    *rxq;            /* packets received */
    dmr_packetq    *txq;            /* packets to be sent */
    dmr_rawq       *rrq;            /* raw frames received */
    dmr_rawq       *trq;            /* raw frames to be sent */
    struct timeval last_ping;       /* last ping sent */
    struct timeval last_pong;       /* last pong received */
} dmr_homebrew;

/** Setup a new Homebrew instance.
 * This function sets up the internal structure as well as the communication
 * sockets, the caller must provide data to the internal config struct before
 * dmr_homebrew_auth is called. */
extern dmr_homebrew * dmr_homebrew_new(dmr_id repeater_id, uint8_t peer_ip[16], uint16_t peer_port, uint8_t bind_ip[16], uint16_t bind_port);

/** Initiate authentication with the repeater. */
extern int dmr_homebrew_auth(dmr_homebrew *homebrew, char *secret);

/** Close the link with the repeater. */
extern int dmr_homebrew_close(dmr_homebrew *homebrew);

/** Read a packet from the repeater.
 * This also processes communications with the Homebrew repeater, if the
 * received frame does not contain a DMR packet, the function will set the
 * destination packet pointer to NULL. */
extern int dmr_homebrew_read(dmr_homebrew *homebrew, dmr_parsed_packet **parsed_out);

/** Send a DMR frame to the repeater. */
extern int dmr_homebrew_send(dmr_homebrew *homebrew, dmr_parsed_packet *parsed);

/** Send a raw buffer to the repeater. */
extern int dmr_homebrew_send_buf(dmr_homebrew *homebrew, uint8_t *buf, size_t len);

/** Send a raw packet to the repeater. */
extern int dmr_homebrew_send_raw(dmr_homebrew *homebrew, dmr_raw *raw);

/** Parse a DMRD frame. */
extern int dmr_homebrew_parse_dmrd(dmr_homebrew *homebrew, dmr_raw *raw, dmr_parsed_packet **parsed_out);

#include <dmr/protocol.h>

/** Protocol specification */
extern dmr_protocol dmr_homebrew_protocol;

#if defined(DMR_TRACE)
#define DMR_HB_TRACE(fmt,...) dmr_log_trace("%s: "fmt, homebrew->id, ##__VA_ARGS__)
#endif
#if defined(DMR_DEBUG)
#define DMR_HB_DEBUG(fmt,...) dmr_log_debug("%s: "fmt, homebrew->id, ##__VA_ARGS__)
#else
#define DMR_HB_DEBUG(fmt,...)
#endif
#define DMR_HB_INFO(fmt,...)  dmr_log_info ("%s: "fmt, homebrew->id, ##__VA_ARGS__)
#define DMR_HB_WARN(fmt,...)  dmr_log_warn ("%s: "fmt, homebrew->id, ##__VA_ARGS__)
#define DMR_HB_ERROR(fmt,...) dmr_log_error("%s: "fmt, homebrew->id, ##__VA_ARGS__)
#define DMR_HB_FATAL(fmt,...) dmr_log_critical("%s: "fmt, homebrew->id, ##__VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif // _DMR_PROTOCOL_HOMEBREW_H
