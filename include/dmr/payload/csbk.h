/**
 * @file
 * @brief Control Block payload.
 */
#ifndef _DMR_PAYLOAD_CSBK_H
#define _DMR_PAYLOAD_CSBK_H

#ifdef __cplusplus
extern "C" {
#endif

#include <dmr/type.h>
#include <dmr/fec/bptc_196_96.h>

typedef enum {
    DMR_CSBK_OUTBOUND_ACTIVATION                 = 0b00111000,
    DMR_CSBK_UNIT_TO_UNIT_VOICE_SERVICE_REQUEST  = 0b00000100,
    DMR_CSBK_UNIT_TO_UNIT_VOICE_SERVICE_RESPONSE = 0b00000101,
    DMR_CSBK_NEGATIVE_ACKNOWLEDGE_RESPONSE       = 0b00100100,
    DMR_CSBK_PREAMBLE                            = 0b00111101
} dmr_csbk_opcode;

typedef struct {
    uint16_t        crc;
    bool            last;
    dmr_csbk_opcode opcode;
    dmr_id          src_id;
    dmr_id          dst_id;
    union {
        struct {
            uint8_t service_options;
        } unit_to_unit_voice_service_request;
        struct {
            uint8_t service_options;
            uint8_t answer_response;
        } unit_to_unit_voice_service_response;
        struct {
            uint8_t source_type		: 1;
			uint8_t service_type	: 6;
			uint8_t reason_code;
        } negative_acknowledge_response;
        struct {
            uint8_t data_follows	: 1;
			uint8_t dst_is_group	: 1;
			uint8_t csbk_blocks_to_follow;
        } preamble;
    } data;
} dmr_csbk;

extern int dmr_csbk_decode(dmr_csbk *csbk, dmr_packet *packet);
extern char *dmr_csbk_opcode_name(dmr_csbk_opcode opcode);

#ifdef __cplusplus
}
#endif

#endif // _DMR_PAYLOAD_CSBK_H
