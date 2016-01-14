/**
 * @file
 */
#ifndef _DMR_PAYLOAD_LC_H
#define _DMR_PAYLOAD_LC_H

#include <dmr/type.h>
#include <dmr/packet.h>
#include <dmr/payload.h>
#include <dmr/fec/rs_12_9.h>

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

#define DMR_FLCO_PDU_GROUP   0x00U
#define DMR_FLCO_PDU_PRIVATE 0x03U

typedef union __attribute__((packed)) {
    struct {
        uint8_t flco            : 6;
        uint8_t reserved        : 1;
        uint8_t protected       : 1;
        uint8_t fid;
        uint8_t service_options;
        uint8_t dst_id[3];
        uint8_t src_id[3];
        uint8_t crc[3];
    } fields;
    uint8_t bytes[12];
} dmr_full_lc_t;

/** Insert Link Control message with Reed-Solomon check data */
extern int dmr_full_lc_encode(dmr_full_lc_t *lc, dmr_packet_t *packet);

#endif // _DMR_PAYLOAD_LC_H
