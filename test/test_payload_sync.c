#include <dmr/payload/sync.h>
#include "_test_header.h"

bool test_all(void) {
    dmr_sync_pattern_t pattern;
    dmr_packet_t *packet = test_packet();

    for (pattern = 0; pattern < DMR_SYNC_PATTERN_UNKNOWN; pattern++) {
        go(dmr_sync_pattern_encode(pattern, packet),   "encode");
        eq(dmr_sync_pattern_decode(packet) == pattern, "sync pattern mismatch");
    }

    return true;
}

static test_t tests[] = {
    {"sync pattern encode & decode", test_all},
    {NULL, NULL} /* sentinel */
};

#include "_test_footer.h"
