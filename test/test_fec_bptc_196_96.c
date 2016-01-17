#include <dmr/bits.h>
#include <dmr/fec/bptc_196_96.h>
#include "_test_header.h"

bool test_all(void)
{
    uint8_t i, j;
    for (i = 0; i < 20; i++) {
        uint8_t data[12], test[12];
        dmr_bptc_196_96_t bptc;
        dmr_packet_t *packet = test_packet();

        for (j = 0; j < 12; j++) {
            data[j] = rand();
        }

        go(dmr_bptc_196_96_encode(&bptc, packet, data), "encode");
        go(dmr_bptc_196_96_decode(&bptc, packet, test), "decode");
        dmr_free(packet);

        if (memcmp(data, test, 12)) {
            printf("input data:\n");
            dmr_dump_hex(data, 12);
            printf("output data:\n");
            dmr_dump_hex(test, 12);
            printf("output corrupt");
            return false;
        }
    }

    return true;
}

static test_t tests[] = {
    {"BPTC (196,96) encode & verify", test_all},
    {NULL, NULL} /* sentinel */
};

#include "_test_footer.h"
