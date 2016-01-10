#include <stdio.h>
#include <stdlib.h>
#if defined(DMR_PLATFORM_WINDOWS)
#include <windows.h>
#endif

#include <dmr/log.h>
#include <dmr/platform.h>
#include <dmr/proto/mmdvm.h>

void list_serial_ports(void)
{
#if defined(DMR_PLATFORM_WINDOWS)
    TCHAR devices[65535];
    unsigned long dwChars = QueryDosDevice(NULL, devices, 65535);
    TCHAR *ptr = devices;

    fprintf(stderr, "\navailable ports:\n\n");
    while (dwChars) {
        if (!strncmp(ptr, "COM", 3)) {
            fprintf(stderr, "\t%s\n", ptr);
        }
        TCHAR *temp_ptr = strchr(ptr, 0);
        dwChars -= (DWORD)((temp_ptr - ptr) / sizeof(TCHAR) + 1);
        ptr = temp_ptr + 1;
    }
#else
#endif
}

int main(int argc, char **argv)
{
    dmr_mmdvm_t *modem;
    long rate = DMR_MMDVM_BAUD_RATE;
    int ret = 0;

    dmr_log_priority_set(DMR_LOG_PRIORITY_VERBOSE);
    if (argc != 2 && argc != 3) {
        fprintf(stderr, "%s <port> [<rate>]\n", argv[0]);
        list_serial_ports();
        return 1;
    }
    if (argc == 3) {
        rate = atol(argv[2]);
    }

    dmr_log_info("mmdvm: on %s rate %ld", argv[1], rate);
    if ((modem = dmr_mmdvm_open(argv[1], rate, 1000UL)) == NULL) {
        return 1;
    }
    if (modem->error != 0) {
        dmr_mmdvm_free(modem);
        return 1;
    }

    /*
    if (!dmr_mmdvm_sync(modem)) {
        dmr_mmdvm_free(modem);
        return 1;
    }
    */

    while (ret == 0) {
        ret = dmr_mmdvm_poll(modem);
    }

    dmr_mmdvm_free(modem);
    return 0;
}
