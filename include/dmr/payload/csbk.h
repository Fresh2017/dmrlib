#ifndef _DMR_PAYLOAD_CSBK_H
#define _DMR_PAYLOAD_CSBK_H

#include <dmr/type.h>
#include <dmrfec/bptc_196_96.h>

#define DMR_CSBK_OUTBOUND_ACTIVATION                 0b00111000
#define DMR_CSBK_UNIT_TO_UNIT_VOICE_SERVICE_REQUEST  0b00000100
#define DMR_CSBK_UNIT_TO_UNIT_VOICE_SERVICE_RESPONSE 0b00000101
#define DMR_CSBK_NEGATIVE_ACKNOWLEDGE_RESPONSE       0b00100100
#define DMR_CSBK_PREAMBLE                            0b00111101
typedef uint8_t dmr_csbk_opcode_t;

typedef struct {
    uint16_t          crc;
    bool              last;
    dmr_csbk_opcode_t opcode;
    dmr_id_t          src_id;
    dmr_id_t          dst_id;
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
} dmr_csbk_t;

extern dmr_csbk_t *dmr_csbk_decode(dmrfec_bptc_196_96_data_bits_t *data_bits);
extern char *dmr_csbk_opcode_name(dmr_csbk_opcode_t opcode);

#endif // _DMR_PAYLOAD_CSBK_H
