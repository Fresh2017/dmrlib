/**
 * @file
 */
#ifndef _DMRLIB_PACKET_H
#define _DMRLIB_PACKET_H

#include <dmr/config.h>
#include <dmr/bits.h>
#include <dmr/type.h>

typedef struct {
    bool bits[216];
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

typedef struct {
    dmr_ts_t        ts;
    dmr_id_t        src_id;
    dmr_id_t        dst_id;
    dmr_id_t        repeater_id;
    dmr_call_type_t call_type;
    dmr_slot_type_t slot_type;
    uint8_t         payload[DMR_PAYLOAD_BYTES];
} dmr_packet_t;

typedef union {
    struct {
        dmr_color_code_t color_code : 4;
        uint8_t          data_type  : 4;
        uint16_t         fec        : 12;
    } fields;
    uint8_t bytes[3];
} dmr_slot_t;

extern dmr_packet_t *dmr_packet_decode(uint8_t *buf, size_t len);
extern char *dmr_slot_type_name(dmr_slot_type_t slot_type);
extern int dmr_payload_bits(dmr_packet_t *packet, void *bits);
extern int dmr_slot_type_decode(dmr_slot_t *slot, dmr_packet_t *packet);
extern int dmr_slot_type_encode(dmr_slot_t *slot, dmr_packet_t *packet);

#endif // _DMRLIB_PACKET_H
