#include <dmr/fec/golay_20_8.h>
#include "_test_header.h"

bool test_all(void)
{
    uint8_t buf[3], i;

    for (i = 0x00; i < 0xff; i ++) {
        buf[0] = i;
        buf[1] = 0;
        buf[2] = 0;
        dmr_golay_20_8_encode(buf);
        eq(dmr_golay_20_8_decode(buf) == buf[0], "decode %02x", i);
    }

    return true;
}


static test_t tests[] = {
    {"Golay (20,8) encode & decode", test_all},
    {NULL, NULL} /* sentinel */
};

#include "_test_footer.h"
