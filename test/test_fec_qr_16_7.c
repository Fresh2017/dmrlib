#include <dmr/fec/qr_16_7.h>
#include "_test_header.h"

bool test_all(void)
{
    uint8_t buf[2], i;

    /* +2 because the last bit is the qr msb */
    for (i = 0x00; i < 0x80; i += 2) {
        buf[0] = i;
        buf[1] = 0;
        dmr_qr_16_7_encode(buf);
        eq(dmr_qr_16_7_decode(buf), "%s %02x", dmr_byte_to_binary(i), i);
    }
    return true;
}

static test_t tests[] = {
    {"Quadratic Residue (16,7,6) encode & decode", test_all},
    {NULL, NULL} /* sentinel */
};

#include "_test_footer.h"
