#ifndef _DMR_PROTO_HOMEBREW_H
#define _DMR_PROTO_HOMEBREW_H

#include <dmr/type.h>
#include <dmr/packet.h>

#define DMR_HOMEBREW_DMR_DATA_SIZE 53

#define DMR_HOMEBREW_UNKNOWN_FRAME  0x00
#define DMR_HOMEBREW_DMR_DATA_FRAME 0x10
typedef uint8_t dmr_homebrew_frame_type_t;

#define DMR_HOMEBREW_DATA_TYPE_VOICE      0x00
#define DMR_HOMEBREW_DATA_TYPE_VOICE_SYNC 0x01
#define DMR_HOMEBREW_DATA_TYPE_DATA_SYNC  0x02

typedef struct {
    char         signature[4];
    uint8_t      sequence;
    dmr_packet_t dmr_packet;
    uint8_t      call_type;
    uint8_t      frame_type;
    uint8_t      data_type;
    uint32_t     stream_id;
} dmr_homebrew_packet_t;

extern dmr_homebrew_frame_type_t dmr_homebrew_frame_type(const uint8_t *bytes, unsigned int len);
extern dmr_homebrew_packet_t *dmr_homebrew_parse_packet(const uint8_t *bytes, unsigned int len);

#endif // _DMR_PROTO_HOMEBREW_H
