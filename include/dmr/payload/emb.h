/**
 * @file
 * @brief Embedded Signalling.
 */
#ifndef _DMR_PAYLOAD_EMB_H
#define _DMR_PAYLOAD_EMB_H

#include <dmr/bits.h>
#include <dmr/type.h>
#include <dmr/packet.h>

#define DMR_EMB_LCSS_SINGLE_FRAGMENT                0b00
#define DMR_EMB_LCSS_FIRST_FRAGMENT                 0b01
#define DMR_EMB_LCSS_LAST_FRAGMENT                  0b10
#define DMR_EMB_LCSS_CONTINUATION                   0b11
typedef uint8_t dmr_emb_lcss_t;

typedef union {
    struct {
        dmr_color_code_t color_code : 4;
        uint8_t          pi         : 1;
        dmr_emb_lcss_t   lcss       : 2;
        uint8_t          qr0        : 1;
        uint8_t          qr1        : 8;
    } flag;
    uint8_t bytes[2];
} dmr_emb_t;

extern int dmr_emb_decode(dmr_emb_t *emb, dmr_packet_t *packet);
extern int dmr_emb_encode(dmr_emb_t *emb, dmr_packet_t *packet);
extern char *dmr_emb_lcss_name(dmr_emb_lcss_t lcss);

#endif // _DMR_PAYLOAD_EMB_H
