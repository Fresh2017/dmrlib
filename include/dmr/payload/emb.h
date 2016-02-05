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

typedef enum {
    DMR_EMB_LCSS_SINGLE_FRAGMENT = 0x00,
    DMR_EMB_LCSS_FIRST_FRAGMENT  = 0x01,
    DMR_EMB_LCSS_LAST_FRAGMENT   = 0x02,
    DMR_EMB_LCSS_CONTINUATION    = 0x03
} dmr_emb_lcss;

typedef struct {
    dmr_color_code color_code;
    bool           pi;
    dmr_emb_lcss   lcss;
    uint16_t       crc;
} dmr_emb;

typedef struct {
    bool bits[72];
    bool checksum[5];
} dmr_emb_signalling_lc_bits;

extern int    dmr_emb_decode(dmr_packet packet, dmr_emb *emb);
extern int    dmr_emb_bytes_decode(dmr_packet packet, uint8_t bytes[4]);
extern int    dmr_emb_encode(dmr_packet packet, dmr_emb *emb);
extern int    dmr_emb_encode_lc(dmr_packet packet, dmr_full_lc *lc, dmr_emb_lcss lcss, uint8_t fragment);
extern bool   dmr_emb_null(uint8_t bytes[4]);
extern char * dmr_emb_lcss_name(dmr_emb_lcss lcss);
extern dmr_emb_signalling_lc_bits * dmr_emb_signalling_lc_interlave(dmr_emb_signalling_lc_bits *emb_bits);
extern int    dmr_emb_encode_signalling_lc_from_full_lc(dmr_full_lc *lc, dmr_emb_signalling_lc_bits *emb_bitss, dmr_data_type data_type);
extern int    dmr_emb_lcss_fragment_encode(dmr_packet packet, dmr_emb *emb, dmr_vbptc_16_11 *vbptc, uint8_t fragment);

#endif // _DMR_PAYLOAD_EMB_H
