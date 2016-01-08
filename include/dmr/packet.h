/**
 * @file
 */
#ifndef _DMRLIB_PACKET_H
#define _DMRLIB_PACKET_H

#include "dmr/bits.h"
#include "dmr/type.h"

typedef struct {
    bit_t bits[216];
} dmr_packet_data_t;

typedef struct {
    uint8_t bytes[27];
} dmr_packet_data_block_bytes_t;

typedef struct {
    uint8_t     serial;
    uint16_t    crc;        // 9 bits CRC
    bool        received_ok;
    uint8_t     data[24];
    uint8_t     data_length;
} dmr_packet_data_block_t;

#include "dmr/payload.h"

typedef struct {
    dmr_ts_t ts;
    dmr_id_t src_id;
    dmr_id_t dst_id;
    dmr_id_t repeater_id;
    dmr_call_type_t call_type;
    dmr_slot_type_t slot_type;
    dmr_payload_t payload;
} dmr_packet_t;

extern char *dmr_packet_get_slot_type_name(dmr_slot_type_t slot_type);

#endif // _DMRLIB_PACKET_H
