/**
 * @file
 */
#ifndef _DMR_PAYLOAD_LC_H
#define _DMR_PAYLOAD_LC_H

#include <dmr/type.h>
#include <dmr/packet.h>
#include <dmr/payload.h>

typedef enum {
    DMR_SO_PRIORITY_NONE,
    DMR_SO_PRIORITY1,
    DMR_SO_PRIORITY2,
    DMR_SO_PRIORITY3
} dmr_so_priority_t;

typedef struct {
    uint8_t emergency          : 1;
    uint8_t privacy            : 1;
    uint8_t reserved           : 2;
    uint8_t broadcast          : 1;
    uint8_t ovcm               : 1;
    dmr_so_priority_t priority : 2;
} dmr_so_t;

typedef enum {
    DMR_FLCO_PDU_GROUP   = 0x00,
    DMR_FLCO_PDU_PRIVATE = 0x03
} dmr_flco_pdu_t;

typedef struct {
    dmr_flco_pdu_t flco_pdu;
    bool           r;
    bool           pf;
    dmr_fid_t      fid;
    uint8_t        service_options;
    dmr_id_t       dst_id;
    dmr_id_t       src_id;
    uint8_t        crc[3];
} dmr_full_lc_t;

extern uint8_t dmr_crc_mask_lc[DMR_DATA_TYPE_COUNT];

extern int dmr_full_lc_decode(dmr_full_lc_t *lc, dmr_packet_t *packet);
extern int dmr_full_lc_encode_bytes(dmr_full_lc_t *lc, uint8_t bytes[12]);
/** Insert Link Control message with Reed-Solomon check data */
extern int dmr_full_lc_encode(dmr_full_lc_t *lc, dmr_packet_t *packet);
extern char *dmr_flco_pdu_name(dmr_flco_pdu_t flco_pdu);

#endif // _DMR_PAYLOAD_LC_H
