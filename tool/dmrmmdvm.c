#include <stdio.h>
#include <stdlib.h>

#include <dmr/proto/mmdvm.h>

int main(int argc, char **argv)
{
    dmr_mmdvm_t *modem;
    long rate = DMR_MMDVM_BAUD_RATE;

    if (argc != 2 && argc != 3) {
        fprintf(stderr, "%s <port> [<rate>]\n", argv[0]);
        return 1;
    }
    if (argc == 3) {
        rate = atol(argv[2]);
    }

    printf("mmdvm: on %s rate %ld\n", argv[1], rate);
    if ((modem = dmr_mmdvm_open(argv[1], rate, 1000UL)) == NULL) {
        return 1;
    }
    if (modem->error != 0) {
        dmr_mmdvm_free(modem);
        return 1;
    }

    if (!dmr_mmdvm_sync(modem)) {
        dmr_mmdvm_free(modem);
        return 1;
    }

    dmr_mmdvm_free(modem);
    return 0;
}
