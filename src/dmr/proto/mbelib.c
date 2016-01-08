#include <inttypes.h>
#include "dmr/proto.h"

void mbelib_process_voice(uint8_t data[33])
{

}

dmr_proto_t mbelib_proto = {
    "mbelib",               /* name */
    NULL,                   /* init */
    NULL,                   /* start */
    NULL,                   /* stop */
    NULL,                   /* active */
    NULL,                   /* process_data */
    mbelib_process_voice,   /* process_voice */
    NULL                    /* relay */
};
