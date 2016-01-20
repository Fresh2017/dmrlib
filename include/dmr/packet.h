/**
 * @file
 */
#ifndef _DMRLIB_PACKET_H
#define _DMRLIB_PACKET_H

#ifdef __cplusplus
extern "C" {
#endif

#include <dmr/config.h>
#include <dmr/bits.h>
#include <dmr/type.h>

typedef enum {
    DMR_TS1 = 0,
    DMR_TS2,
    DMR_TS_INVALID
} dmr_ts_t;

typedef uint32_t dmr_id_t;

typedef uint8_t dmr_color_code_t;

typedef enum {
    DMR_FLCO_GROUP = 0x00,
    DMR_FLCO_PRIVATE,
    DMR_FLCO_INVALID
} dmr_flco_t;

typedef enum {
    DMR_FID_ETSI     = 0x00,
    DMR_FID_DMRA     = 0x0b,
    DMR_FID_HYTERA   = 0x08,
    DMR_FID_MOTOROLA = 0x10
} dmr_fid_t;

typedef enum {
    DMR_DATA_TYPE_VOICE_PI = 0x00,
    DMR_DATA_TYPE_VOICE_LC,
    DMR_DATA_TYPE_TERMINATOR_WITH_LC,
    DMR_DATA_TYPE_CSBK,
    DMR_DATA_TYPE_MBC,
    DMR_DATA_TYPE_MBCC,
    DMR_DATA_TYPE_DATA,
    DMR_DATA_TYPE_RATE12_DATA,
    DMR_DATA_TYPE_RATE34_DATA,
    DMR_DATA_TYPE_IDLE,
    /* pseudo data types */
    DMR_DATA_TYPE_INVALID,
    DMR_DATA_TYPE_SYNC_VOICE = 0x20,    /* for MMDVM modems */
    DMR_DATA_TYPE_VOICE_SYNC = 0xf0,
    DMR_DATA_TYPE_VOICE      = 0xf1
} dmr_data_type_t;

typedef struct {
    dmr_ts_t         ts;
    dmr_flco_t       flco;
    dmr_id_t         src_id;
    dmr_id_t         dst_id;
    dmr_id_t         repeater_id;
    dmr_data_type_t  data_type;
    dmr_color_code_t color_code;
    uint8_t          payload[DMR_PAYLOAD_BYTES];
    /* not part of the air interface */
    struct {
        uint8_t  sequence;
        uint8_t  voice_frame;
        uint32_t stream_id;
    } meta;
} dmr_packet_t;

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

extern void dmr_dump_packet(dmr_packet_t *packet);
extern dmr_packet_t *dmr_packet_decode(uint8_t *buf, size_t len);
extern char *dmr_flco_name(dmr_flco_t flco);
extern char *dmr_ts_name(dmr_ts_t ts);
extern char *dmr_fid_name(dmr_fid_t fid);
extern char *dmr_data_type_name(dmr_data_type_t data_type);
extern char *dmr_data_type_name_short(dmr_data_type_t data_type);
extern int dmr_payload_bits(dmr_packet_t *packet, void *bits);
extern int dmr_slot_type_decode(dmr_packet_t *packet);
extern int dmr_slot_type_encode(dmr_packet_t *packet);

#ifdef __cplusplus
}
#endif

#endif // _DMRLIB_PACKET_H
