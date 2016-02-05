/**
 * @file   Homebrew IPSC protocol (Brandmeister)
 * @brief  This implements the Homebrew IP Site Connect protocol as offered by
 *         DMR repeaters running the Brandmeister software stack.
 * @author Wijnand Modderman-Lenstra PD0MZ
 */
#ifndef _DMR_PROTOCOL_HOMEBREW_H
#define _DMR_PROTOCOL_HOMEBREW_H

#include <inttypes.h>
#include <dmr/config.h>
#include <dmr/packet.h>
#include <dmr/packetq.h>
#include <dmr/protocol.h>

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
    char        *peer_addr;
    uint16_t    peer_port;
    char        *bind_addr;
    uint16_t    bind_port;
    struct {
        char           *call;
        dmr_id         repeater_id;
        uint16_t       rx_freq;
        uint16_t       tx_freq;
        uint8_t        tx_power;
        dmr_color_code color_code;
        double         latitude;
        double         longitude;
        uint8_t        altitude;
        char           *location;
        char           *description;
        char           *url;
        char           *software_id;
        char           *package_id;
    } config;
    /* private */
    void        *sock;
    uint8_t     peer_ip[16];
    uint8_t     bind_ip[16];
    uint8_t     state;
    dmr_packetq *rxq;           /* packets received */
    dmr_packetq *txq;           /* packets to be sent */
    void        *buffer;        /* internal buffer */
} dmr_homebrew;

/** Setup a new Homebrew instance.
 * This function sets up the internal structure as well as the communication
 * sockets, the caller must provide data to the internal config struct before
 * dmr_homebrew_auth is called. */
extern dmr_homebrew * dmr_homebrew_new(char *peer_addr, uint16_t peer_port, char *bind_addr, uint16_t bind_port);
extern int            dmr_homebrew_auth(dmr_homebrew *homebrew, char *secret);


#ifdef __cplusplus
}
#endif

#endif // _DMR_PROTOCOL_HOMEBREW_H
