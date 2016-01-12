/**
 * @file
 */
#ifndef _DMR_PAYLOAD_LC_H
#define _DMR_PAYLOAD_LC_H

#include <dmr/type.h>
#include <dmr/payload.h>
#include <dmr/fec/rs_12_9.h>

typedef enum {
    DMR_LC_GROUP_VOICE_CHANNEL_USER        = 0x00U,  // B00000000
    DMR_LC_UNIT_TO_UNIT_VOICE_CHANNEL_USER = 0x03U,  // B00000011
} dmr_flco_t;

typedef enum {
    DMR_SO_PRIORITY_NONE,
    DMR_SO_PRIORITY1,
    DMR_SO_PRIORITY2,
    DMR_SO_PRIORITY3
} dmr_so_priority_t;

typedef struct {
    dmr_flco_t flco;
    uint8_t    fid;
    union {
        struct {
            uint8_t emergency          : 1;
            uint8_t privacy            : 1;
            uint8_t reserved           : 2;
            uint8_t broadcast          : 1;
            uint8_t ovcm               : 1;
            dmr_so_priority_t priority : 2;
        } flags;
        uint8_t byte;
    } service_options;
    dmr_id_t src_id;
    dmr_id_t dst_id;
} dmr_lc_t;

/** Insert Link Control message with Reed-Solomon check data */
extern int dmr_lc_full_encode(dmr_lc_t *lc, dmr_packet_t *packet);

#endif // _DMR_PAYLOAD_LC_H
