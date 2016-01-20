#include <dmr/packet.h>

#include "_test_header.h"

bool test_encode(void) {
    dmr_packet_t *packet = test_packet();
    uint8_t i;

    for (i = DMR_DATA_TYPE_VOICE_LC; i < DMR_DATA_TYPE_INVALID; i++) {
        memset(packet->payload, 0x11 * i, 33);

        packet->color_code = 1;
        packet->data_type = i;
        if (dmr_slot_type_encode(packet) != 0) {
            printf("encode failed");
            return false;
        }
        printf("%s .. ", dmr_data_type_name_short(packet->data_type));
        if (packet->data_type != i) {
            printf("expected %02x, got %02x", DMR_DATA_TYPE_VOICE_LC, packet->data_type);
            return false;
        }
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
