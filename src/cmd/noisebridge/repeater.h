#ifndef _NOISEBRIDGE_REPEATER_H
#define _NOISEBRIDGE_REPEATER_H

#include <dmr/io.h>
#include <dmr/protocol.h>

typedef enum {
    ROUTE_REJECT = 0x00,
    ROUTE_PERMIT,
    ROUTE_PERMIT_UNMODIFIED
} route_policy;

typedef enum {
    STATE_IDLE = 0x00,
    STATE_DATA_CALL,
    STATE_VOICE_CALL,
    STATES
} slot_state;

typedef struct {
    dmr_data_type  data_type;
    dmr_id         src_id;
    dmr_id         dst_id;
    slot_state     state;
    uint32_t       stream_id;
    struct timeval last_frame_received;
} repeater_slot_t;

typedef struct {
    repeater_slot_t ts[2];
    dmr_color_code  color_code;
    dmr_io          *io;
} repeater_t;

typedef route_policy (*repeater_route)(repeater_t *, proto_t *, proto_t *, dmr_parsed_packet *);

repeater_t *load_repeater(void);
int init_repeater(void);
int loop_repeater(void);

#endif // _NOISEBRIDGE_REPEATER_H
