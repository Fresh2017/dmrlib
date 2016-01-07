#ifndef _DMR_TYPE_H
#define _DMR_TYPE_H

#include <stddef.h>
#include <stdbool.h>
#include <inttypes.h>

#define DMR_CALL_TYPE_PRIVATE           0x00
#define DMR_CALL_TYPE_GROUP             0x01
typedef uint8_t dmr_call_type_t;

#define DMR_DATA_TYPE_UNKNOWN			0x00
#define DMR_DATA_TYPE_NORMAL_SMS		0x01
#define DMR_DATA_TYPE_MOTOROLA_TMS_SMS	0x02
#define DMR_DATA_TYPE_GPS_POSITION		0x03
typedef uint8_t dmr_data_type_t;

#define DMR_SLOT_TYPE_PRIVACY_INDICATOR         0x00U
#define DMR_SLOT_TYPE_VOICE_LC                  0x01U
#define DMR_SLOT_TYPE_TERMINATOR_WITH_LC        0x02U
#define DMR_SLOT_TYPE_CSBK                      0x03U
#define DMR_SLOT_TYPE_MULTI_BLOCK_CONTROL       0x04U
#define DMR_SLOT_TYPE_MULTI_BLOCK_CONTROL_CONT  0x05U
#define DMR_SLOT_TYPE_DATA                      0x06U
#define DMR_SLOT_TYPE_RATE12_DATA               0x07U
#define DMR_SLOT_TYPE_RATE34_DATA               0x08U
#define DMR_SLOT_TYPE_IDLE                      0x09U
#define DMR_SLOT_TYPE_VOICE_BURST_A             0x0aU
#define DMR_SLOT_TYPE_VOICE_BURST_B             0x0bU
#define DMR_SLOT_TYPE_VOICE_BURST_C             0x0cU
#define DMR_SLOT_TYPE_VOICE_BURST_D             0x0dU
#define DMR_SLOT_TYPE_VOICE_BURST_E             0x0eU
#define DMR_SLOT_TYPE_VOICE_BURST_F             0x0fU
#define DMR_SLOT_TYPE_SYNC_VOICE                0x20U
#define DMR_SLOT_TYPE_SYNC_DATA                 0x40U
#define DMR_SLOT_TYPE_IDLE_RX                   0x80U
#define DMR_SLOT_TYPE_UNKNOWN                   0xffU
typedef uint8_t dmr_slot_type_t;

#define DMR_TS1                         0x00
#define DMR_TS2                         0x01
typedef uint8_t dmr_ts_t;

typedef uint32_t dmr_id_t;
typedef uint8_t dmr_color_code_t;

#endif // _DMR_TYPE_H
