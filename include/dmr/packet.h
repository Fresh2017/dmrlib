/**
 * @file
 */
#ifndef _DMRLIB_PACKET_H
#define _DMRLIB_PACKET_H

#include <dmr/config.h>
#include <dmr/bits.h>
#include <dmr/type.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DMR_PACKET_LEN      33
#define DMR_PACKET_BITS     (DMR_PACKET_LEN * 8)

typedef enum {
    DMR_TS1 = 0,
    DMR_TS2,
    DMR_TS_INVALID
} dmr_ts;

typedef uint32_t dmr_id;

typedef uint8_t dmr_color_code;

typedef enum {
    DMR_FLCO_GROUP = 0x00,
    DMR_FLCO_PRIVATE,
    DMR_FLCO_INVALID
} dmr_flco;

typedef enum {
    DMR_FID_ETSI     = 0x00,
    DMR_FID_DMRA     = 0x0b,
    DMR_FID_HYTERA   = 0x08,
    DMR_FID_MOTOROLA = 0x10
} dmr_fid;

typedef enum {
    DMR_DATA_TYPE_VOICE_PI = 0x00,
    DMR_DATA_TYPE_VOICE_LC,
    DMR_DATA_TYPE_TERMINATOR_WITH_LC,
    DMR_DATA_TYPE_CSBK,
    DMR_DATA_TYPE_MBC_HEADER,
    DMR_DATA_TYPE_MBC_CONTINUATION,
    DMR_DATA_TYPE_DATA_HEADER,
    DMR_DATA_TYPE_RATE12_DATA,
    DMR_DATA_TYPE_RATE34_DATA,
    DMR_DATA_TYPE_IDLE,
    /* pseudo data types */
    DMR_DATA_TYPE_INVALID,
    DMR_DATA_TYPE_SYNC_VOICE = 0x20,    /* for MMDVM modems */
    DMR_DATA_TYPE_VOICE_SYNC = 0xf0,
    DMR_DATA_TYPE_VOICE      = 0xf1
} dmr_data_type;

#define DMR_DATA_TYPE_COUNT DMR_DATA_TYPE_INVALID

typedef uint8_t dmr_packet[DMR_PACKET_LEN];

typedef struct {
    dmr_packet     packet;
    dmr_ts         ts;
    dmr_flco       flco;
    dmr_id         src_id, dst_id, repeater_id;
    dmr_data_type  data_type;
    dmr_color_code color_code;
    uint8_t        sequence;
    uint32_t       stream_id;
    uint8_t        voice_frame;
    bool           parsed;
} dmr_parsed_packet;

typedef struct {
    bool bits[216];
} dmr_packet_data;

typedef struct {
    uint8_t bytes[27];
} dmr_packet_data_block_bytes;

typedef struct {
    uint8_t     serial;
    uint16_t    crc;        // 9 bits CRC
    bool        received_ok;
    uint8_t     data[24];
    uint8_t     data_length;
} dmr_packet_data_block;

typedef int (*dmr_parsed_packet_cb)(dmr_parsed_packet *parsed, void *userdata);

typedef int (*dmr_packet_cb)(dmr_packet packet, void *userdata);

extern void                dmr_dump_packet(dmr_packet packet);
extern void                dmr_dump_parsed_packet(dmr_parsed_packet *packet);
extern dmr_parsed_packet * dmr_packet_decode(dmr_packet packet);
extern char *              dmr_flco_name(dmr_flco flco);
extern char *              dmr_ts_name(dmr_ts ts);
extern char *              dmr_fid_name(dmr_fid fid);
extern char *              dmr_data_type_name(dmr_data_type data_type);
extern char *              dmr_data_type_name_short(dmr_data_type data_type);
extern int                 dmr_payload_bits(dmr_packet packet, void *bits);
extern int                 dmr_slot_type_decode(dmr_packet packet, dmr_color_code *color_code, dmr_data_type *data_type);
extern int                 dmr_slot_type_encode(dmr_packet packet, dmr_color_code color_code, dmr_data_type data_type);

#ifdef __cplusplus
}
#endif

#endif // _DMRLIB_PACKET_H
