#include "dmr/payload/csbk.h"
#include "dmrfec/bptc_196_96.h"

dmr_csbk_t *dmr_csbk_decode(dmrfec_bptc_196_96_data_bits_t *data_bits)
{
    static dmr_csbk_t csbk;
    uint8_t bytes[12];

    if (data_bits == NULL)
        return NULL;

    bits_to_bytes(data_bits->bits, sizeof(dmrfec_bptc_196_96_data_bits_t), bytes, 12);

    // Protect flag is set.
    if (bytes[0] & 0b01000000)
        return NULL;

    // Feature set ID is set.
    if (bytes[1] != 0)
        return NULL;

    csbk.last = (bytes[0] & 0b10000000) > 0;
    csbk.dst_id = bytes[4] << 16 | bytes[5] << 8 | bytes[6];
    csbk.src_id = bytes[7] << 16 | bytes[8] << 8 | bytes[9];
    csbk.opcode = bytes[0] & 0b111111;

    switch (csbk.opcode) {
    case DMR_CSBK_OUTBOUND_ACTIVATION:
        break;
    case DMR_CSBK_UNIT_TO_UNIT_VOICE_SERVICE_REQUEST:
        csbk.data.unit_to_unit_voice_service_request.service_options = bytes[2];
        break;
    case DMR_CSBK_UNIT_TO_UNIT_VOICE_SERVICE_RESPONSE:
        csbk.data.unit_to_unit_voice_service_response.service_options = bytes[2];
        csbk.data.unit_to_unit_voice_service_response.answer_response = bytes[3];
        break;
    case DMR_CSBK_NEGATIVE_ACKNOWLEDGE_RESPONSE:
        csbk.data.negative_acknowledge_response.source_type = ((bytes[2] & 0b01000000) > 0);
        csbk.data.negative_acknowledge_response.service_type = (bytes[2] & 0b00111111);
        csbk.data.negative_acknowledge_response.reason_code = bytes[3];
        break;
    case DMR_CSBK_PREAMBLE:
        csbk.data.preamble.data_follows = ((bytes[2] & 0b10000000) > 0);
        csbk.data.preamble.dst_is_group = ((bytes[2] & 0b01000000) > 0);
        csbk.data.preamble.csbk_blocks_to_follow = bytes[3];
        break;
    default:
        return NULL; // Invalid opcode
    }

    return &csbk;
}

char *dmr_csbk_opcode_name(dmr_csbk_opcode_t opcode)
{
    switch (opcode) {
    case DMR_CSBK_OUTBOUND_ACTIVATION:
        return "outbound activation";
    case DMR_CSBK_UNIT_TO_UNIT_VOICE_SERVICE_REQUEST:
        return "unit to unit voice service request";
    case DMR_CSBK_UNIT_TO_UNIT_VOICE_SERVICE_RESPONSE:
        return "unit to unit voice service response";
    case DMR_CSBK_NEGATIVE_ACKNOWLEDGE_RESPONSE:
        return "negative acknowledge response";
    case DMR_CSBK_PREAMBLE:
        return "preamble";
    default:
        return "unknown";
    }
}
