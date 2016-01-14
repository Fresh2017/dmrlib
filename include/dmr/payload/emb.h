/**
 * @file
 * @brief Embedded Signalling.
 */
#ifndef _DMR_PAYLOAD_EMB_H
#define _DMR_PAYLOAD_EMB_H

#include <dmr/bits.h>
#include <dmr/type.h>
#include <dmr/packet.h>
#include <dmr/payload/lc.h>
#include <dmr/fec/vbptc_16_11.h>

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
    } flags;
    uint8_t bytes[2];
} dmr_emb_t;

typedef struct {
    bool bits[72];
    bool checksum[5];
} dmr_emb_signalling_lc_bits_t;

extern int dmr_emb_decode(dmr_emb_t *emb, dmr_packet_t *packet);
extern int dmr_emb_encode(dmr_emb_t *emb, dmr_packet_t *packet);
extern int dmr_emb_encode_lc(dmr_full_lc_t *lc, dmr_emb_lcss_t lcss, uint8_t fragment, dmr_packet_t *packet);
extern char *dmr_emb_lcss_name(dmr_emb_lcss_t lcss);
extern dmr_emb_signalling_lc_bits_t *dmr_emb_signalling_lc_interlave(dmr_emb_signalling_lc_bits_t *emb_bits);
extern int dmr_emb_encode_signalling_lc_from_full_lc(dmr_full_lc_t *lc, dmr_emb_signalling_lc_bits_t *emb_bits);
extern int dmr_emb_lcss_fragment_encode(dmr_emb_lcss_t lcss, dmr_vbptc_16_11_t *vbptc, uint8_t fragment, dmr_packet_t *packet);

#endif // _DMR_PAYLOAD_EMB_H
