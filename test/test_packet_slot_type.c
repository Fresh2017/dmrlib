#include <dmr/packet.h>

#include "_test_header.h"

bool test_encode(void) {
    dmr_packet_t *packet = test_packet();
    packet->color_code = 1;
    packet->data_type = DMR_DATA_TYPE_VOICE_LC;
    if (dmr_slot_type_encode(packet) != 0) {
        printf("encode failed");
        return false;
    }
    if (packet->data_type != DMR_DATA_TYPE_VOICE_LC) {
        printf("expected %02x, got %02x", DMR_DATA_TYPE_VOICE_LC, packet->data_type);
        return false;
    }

    /* should fail */
    packet->color_code = 0;
    if (dmr_slot_type_encode(packet) == 0) {
        printf("encode color_code 0 did not fail");
        return false;
    }

    return true;
}

bool test_encode_decode(void) {
    dmr_packet_t *packet = test_packet();
    packet->color_code = 1;
    packet->data_type = DMR_DATA_TYPE_VOICE_LC;

    if (dmr_slot_type_encode(packet) != 0) {
        printf("encode failed");
        return false;
    }

    if (dmr_slot_type_decode(packet) != 0) {
        printf("decode failed");
        return false;
    }
    return true;
}

static test_t tests[] = {
    {"slot type encode", test_encode},
    {"slot type encode & decode", test_encode_decode},
    {NULL, NULL} /* sentinel */
};

#include "_test_footer.h"
