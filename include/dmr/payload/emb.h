#ifndef _DMR_PAYLOAD_EMB_H
#define _DMR_PAYLOAD_EMB_H

#include <dmr/type.h>
#include <dmr/payload/sync.h>

#define DMR_EMB_LCSS_SINGLE_FRAGMENT                0b00
#define DMR_EMB_LCSS_FIRST_FRAGMENT                 0b01
#define DMR_EMB_LCSS_LAST_FRAGMENT                  0b10
#define DMR_EMB_LCSS_CONTINUATION                   0b11
typedef uint8_t dmr_emb_lcss_t;

typedef struct {
    dmr_color_code_t color_code;
    dmr_emb_lcss_t lcss;
} dmr_emb_t;

typedef struct {
    bit_t bits[16];
} dmr_emb_bits_t;

typedef struct {
	bit_t bits[72];
	bit_t checksum[5];
} dmr_emb_signalling_lc_bits_t;

typedef struct {
    bit_t bits[32];
} dmr_emb_signalling_lc_fragment_bits_t;

extern dmr_emb_bits_t *dmr_emb_from_payload_sync_bits(dmr_payload_sync_bits_t *sync_bits);
extern dmr_emb_t *dmr_emb_decode(dmr_emb_bits_t *emb_bits);
extern char *dmr_emb_lcss_name(dmr_emb_lcss_t lcss);

#endif // _DMR_PAYLOAD_EMB_H
