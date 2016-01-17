#include <dmr/fec/reed_solomon.h>
#include <dmr/crc.h>
#include "_test_header.h"

bool test_init(void)
{
    go(dmr_rs_init(), "init");
    return true;
}

bool test_simple(void)
{
    uint8_t i, j;
    for (i = 0; i < 20; i++) {
        uint8_t bytes[12];
        memset(bytes, 0, 12);
        for (j = 0; j < 9; j++) {
            bytes[j] = rand();
        }
        dmr_log_debug("%s: generated random data:", prog);
        dmr_dump_hex(bytes, 12);
        dmr_rs_12_9_4_encode(bytes, DMR_CRC8_MASK_VOICE_LC);
        dmr_log_debug("%s: generated Reed-Solomon(12,9,4) parities:", prog);
        dmr_dump_hex(bytes, 12);
        go(dmr_rs_12_9_4_decode_and_verify(bytes, DMR_CRC8_MASK_VOICE_LC), "decode and verify");
        //eq(dmr_rs_12_9_verify(&codeword) == 0, "verify");
    }
    return true;
}

static test_t tests[] = {
    {"Reed-Solomon(12,9,4) init", test_init},
    {"Reed-Solomon(12,9,4) encode & decode (verify)", test_simple},
    {NULL, NULL} /* sentinel */
};

#include "_test_footer.h"
