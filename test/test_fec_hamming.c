#include <dmr/fec/hamming.h>
#include "_test_header.h"

void fill_random(bool *buf, size_t len)
{
    uint16_t random = rand();
    size_t i;
    for (i = 0; i < len; i++) {
        buf[i] = (random & 1) == 1;
        random >>= 1;
    }
}

bool test_hamming_13_9_3(void)
{
    uint8_t i;
    for (i = 0; i < 20; i++) {
        bool bits[13], parity[4];
        fill_random(bits, sizeof(bits) - 4);
        dmr_hamming_13_9_3_encode_bits(bits, bits + 9);
        eq(dmr_hamming_13_9_3_verify_bits(bits, parity), "verify");
    }

    return true;
}

bool test_hamming_15_11_3(void)
{
    uint8_t i;
    for (i = 0; i < 20; i++) {
        bool bits[15], parity[4];
        fill_random(bits, sizeof(bits) - 4);
        dmr_hamming_15_11_3_encode_bits(bits, bits + 11);
        eq(dmr_hamming_15_11_3_verify_bits(bits, parity), "verify");
    }

    return true;
}

bool test_hamming_16_11_4(void)
{
    uint8_t i;
    for (i = 0; i < 20; i++) {
        bool bits[16], parity[5];
        fill_random(bits, sizeof(bits) - 5);
        dmr_hamming_16_11_4_encode_bits(bits, bits + 11);
        eq(dmr_hamming_16_11_4_verify_bits(bits, parity), "verify");
    }

    return true;
}

bool test_hamming_17_12_3(void)
{
    uint8_t i;
    for (i = 0; i < 20; i++) {
        bool bits[16], parity[5];
        fill_random(bits, sizeof(bits) - 5);
        dmr_hamming_17_12_3_encode_bits(bits, bits + 12);
        eq(dmr_hamming_17_12_3_verify_bits(bits, parity), "verify");
    }

    return true;
}

static test_t tests[] = {
    {"Hamming (13,9,3) encode & verify",  test_hamming_13_9_3 },
    {"Hamming (15,11,3) encode & verify", test_hamming_15_11_3},
    {"Hamming (16,11,4) encode & verify", test_hamming_16_11_4},
    {"Hamming (17,12,3) encode & verify", test_hamming_17_12_3},
    {NULL, NULL} /* sentinel */
};

#include "_test_footer.h"
