#ifndef _DMR_PROTO_H
#define _DMR_PROTO_H

#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>

#include <dmr/type.h>

typedef bool (*dmr_proto_init_t)(void *);
typedef bool (*dmr_proto_start_t)(void *);
typedef bool (*dmr_proto_stop_t)(void *);
typedef bool (*dmr_proto_active_t)(void *);
typedef void (*dmr_proto_process_data_t)(uint8_t *, size_t);
typedef void (*dmr_proto_process_voice_t)(uint8_t[33]);
typedef void (*dmr_proto_relay_t)(dmr_ts_t, uint8_t[33]);

typedef struct {
    const char                *name;
    dmr_proto_init_t          init;
    dmr_proto_start_t         start;
    dmr_proto_stop_t          stop;
    dmr_proto_active_t        active;
    dmr_proto_process_data_t  process_data;
    dmr_proto_process_voice_t process_voice;
    dmr_proto_relay_t         relay;
} dmr_proto_t;

#endif // _DMR_PROTO_H
