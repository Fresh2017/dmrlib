#include <dmr/payload/emb.h>
#include "_test_header.h"

bool test_encode(void) {
    dmr_packet_t *packet = test_packet();
    dmr_emb_t emb = {
        .color_code = 1,
        .pi         = true,
        .lcss       = DMR_EMB_LCSS_LAST_FRAGMENT,
        .crc        = 0
    };

    if (dmr_emb_encode(&emb, packet) != 0) {
        printf("encode failed");
        return false;
    }

    return true;
}

bool test_encode_decode(void) {
    dmr_packet_t *packet = test_packet();
    dmr_emb_t emb = {
        .color_code = 1,
        .pi         = true,
        .lcss       = DMR_EMB_LCSS_LAST_FRAGMENT,
        .crc        = 0
    }, decode_emb;

    if (dmr_emb_encode(&emb, packet) != 0) {
        printf("encode failed");
        return false;
    }

    go(dmr_emb_decode(&decode_emb, packet),      "decode failed");        
    eq((emb.color_code == 1),                    "color_code");
    eq((emb.pi             ),                    "pi");
    eq((emb.lcss == DMR_EMB_LCSS_LAST_FRAGMENT), "lcss");

    return true;
}

static test_t tests[] = {
    {"emb encode", test_encode},
    {"emb encode & decode", test_encode_decode},
    {NULL, NULL} /* sentinel */
};

#include "_test_footer.h"
