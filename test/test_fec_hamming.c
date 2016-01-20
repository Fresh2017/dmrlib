#include <dmr/fec/hamming.h>
#include "_test_header.h"

void fill_random(bool *buf, size_t len)
{
    uint16_t random = rand();
    size_t i;
    for (i = 0; i < len; i++) {
        buf[i] = (random & 1);
        random >>= 1;
    }
}

#define HAMMING_CODE_DMR_NAME(b,fn) dmr_##b##_##fn
#define HAMMING_CODE_NAME(b)        test_##b
#define HAMMING_CODE(b,_s)          bool HAMMING_CODE_NAME(b)(void) { \
    uint8_t i; \
    for (i = 0; i < 20; i++) { \
        bool bits[_s]; \
        fill_random(bits, sizeof(bits) - 4); \
        HAMMING_CODE_DMR_NAME(b,encode)(bits); \
        eq(HAMMING_CODE_DMR_NAME(b,decode)(bits), "decode"); \
    } \
    return true; \
}

/*
bool test_hamming_13_9_3_parity(void)
{
    uint8_t i, p;
    for (i = 0; i < 20; i++) {
        uint8_t bits[13];
        fill_random(bits, sizeof(bits) - 4);
        dmr_hamming_13_9_3_encode(bits);
        p = 1 + rand() % 8;
        dmr_log_debug("corrupting byte %u", p);
        bits[p] = !bits[p];
        eq(dmr_hamming_13_9_3_decode(bits), "decode");
    }

    return true;
}
*/


HAMMING_CODE(hamming_7_4_3, 7);
HAMMING_CODE(hamming_13_9_3, 13);
HAMMING_CODE(hamming_15_11_3, 15);
HAMMING_CODE(hamming_16_11_4, 16);
HAMMING_CODE(hamming_17_12_3, 17);

static test_t tests[] = {
    {"Hamming(7,4,3) encode & verify", test_hamming_7_4_3},
    {"Hamming(13,9,3) encode & verify",  test_hamming_13_9_3},
    {"Hamming(15,11,3) encode & verify", test_hamming_15_11_3},
    {"Hamming(16,11,4) encode & verify", test_hamming_16_11_4},
    {"Hamming(17,12,3) encode & verify", test_hamming_17_12_3},
    {NULL, NULL} /* sentinel */
};

#include "_test_footer.h"
