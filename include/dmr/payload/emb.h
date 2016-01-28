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

#define DMR_EMB_LCSS_SINGLE_FRAGMENT                0x00
#define DMR_EMB_LCSS_FIRST_FRAGMENT                 0x01
#define DMR_EMB_LCSS_LAST_FRAGMENT                  0x02
#define DMR_EMB_LCSS_CONTINUATION                   0x03
typedef uint8_t dmr_emb_lcss_t;

typedef struct {
    dmr_color_code_t color_code;
    bool             pi;
    dmr_emb_lcss_t   lcss;
    uint16_t         crc;
} dmr_emb_t;

typedef struct {
    bool bits[72];
    bool checksum[5];
} dmr_emb_signalling_lc_bits_t;

extern int dmr_emb_decode(dmr_emb_t *emb, dmr_packet_t *packet);
extern int dmr_emb_bytes_decode(uint8_t bytes[4], dmr_packet_t *packet);
extern int dmr_emb_encode(dmr_emb_t *emb, dmr_packet_t *packet);
extern int dmr_emb_encode_lc(dmr_full_lc_t *lc, dmr_emb_lcss_t lcss, uint8_t fragment, dmr_packet_t *packet);
extern bool dmr_emb_null(uint8_t bytes[4]);
extern char *dmr_emb_lcss_name(dmr_emb_lcss_t lcss);
extern dmr_emb_signalling_lc_bits_t *dmr_emb_signalling_lc_interlave(dmr_emb_signalling_lc_bits_t *emb_bits);
extern int dmr_emb_encode_signalling_lc_from_full_lc(dmr_full_lc_t *lc, dmr_emb_signalling_lc_bits_t *emb_bitss, dmr_data_type_t data_type);
extern int dmr_emb_lcss_fragment_encode(dmr_emb_t *emb, dmr_vbptc_16_11_t *vbptc, uint8_t fragment, dmr_packet_t *packet);

#endif // _DMR_PAYLOAD_EMB_H
