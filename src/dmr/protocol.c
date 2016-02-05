#include <talloc.h>
#include "dmr/error.h"
#include "dmr/protocol.h"
#include "common/byte.h"

DMR_API dmr_protocol *dmr_protocol_init(dmr_protocol template, void *instance)
{
    dmr_protocol *protocol = talloc_zero(NULL, dmr_protocol);
    if (protocol == NULL) {
        DMR_OOM();
        return NULL;
    }

    byte_copy(protocol, &template, sizeof template);
    protocol->instance = instance;
    return protocol;
}
