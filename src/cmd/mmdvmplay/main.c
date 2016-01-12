#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
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

struct timeval tv;
unsigned long gettime_ms(void)
{
    gettimeofday(&tv, NULL);
    return 1000000 * tv.tv_sec + tv.tv_usec;
}

int main(int argc, char **argv)
{
    dmr_mmdvm_t *modem;
    long rate = DMR_MMDVM_BAUD_RATE;
    FILE *fp;
    int ret = 0, n;
    uint8_t buf[37];

    dmr_log_priority_set(DMR_LOG_PRIORITY_TRACE);
    if (argc != 3) {
        fprintf(stderr, "%s <port> <file>\n", argv[0]);
        list_serial_ports();
        return 1;
    }
    fp = fopen(argv[2], "r");
    if (fp == NULL) {
        dmr_log_critical("mmdvm: error opening %s: %s", argv[2], strerror(errno));
        return 1;
    }

    dmr_log_info("mmdvm: on %s rate %ld", argv[1], rate);
    if ((modem = dmr_mmdvm_open(argv[1], rate, 0, 1000UL)) == NULL) {
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

    unsigned long start, stop;
    while (ret == 0) {
        n = fread(buf, sizeof(buf[0]), sizeof(buf) / sizeof(buf[0]), fp);
        if (n <= 0) {
            if (feof(fp)) {
                dmr_log_info("mmdvm: %s EOF at %ld\n", argv[2], ftell(fp));
                break;
            }
            dmr_log_error("mmdvm: reading %s: %s", argv[2], strerror(errno));
            break;
        }
        dmr_log_debug("sending %d bytes:\n", n);
        dump_hex(buf, n);
        start = gettime_ms();
        if (dmr_serial_write(modem->fd, buf, n) != n) {
            dmr_log_error("mmdvm: failed writing %d bytes", n);
            break;
        }
        stop = gettime_ms();
        usleep(50000 - (stop - start));
        dmr_log_debug("mmdvm: delay %lu", gettime_ms() - start);
        //ret = dmr_mmdvm_poll(modem);
    }

    dmr_mmdvm_free(modem);
    return 0;
}
