#include <dmr/fec/rs_12_9.h>
#include <dmr/crc.h>
#include "_test_header.h"

bool test_simple(void)
{
    uint8_t i, j;
    for (i = 0; i < 20; i++) {
        uint8_t bytes[12];
        memset(bytes, 0, 12);
        for (j = 0; j < 9; j++) {
            bytes[j] = rand();
        }
        dmr_rs_12_9_4_encode(bytes, DMR_CRC8_MASK_VOICE_LC);
        go(dmr_rs_12_9_4_decode(bytes, DMR_CRC8_MASK_VOICE_LC), "decode and verify");
    }
    return true;
}

static test_t tests[] = {
    {"Reed-Solomon(12,9,4) encode & decode (verify)", test_simple},
    {NULL, NULL} /* sentinel */
};

#include "_test_footer.h"
