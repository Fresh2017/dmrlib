#include <getopt.h>
#include <stdio.h>
#include <stdarg.h>
#include <dmr.h>
#include <dmr/id.h>
#include <dmr/thread.h>
#include <dmr/thread.h>
#include "common/serial.h"
#include "config.h"
#include "http.h"
#include "repeater.h"
#include "script.h"

static struct option long_options[] = {
    {"config", required_argument, NULL, 'c'},
    {NULL, 0, NULL, 0} /* Sentinel */
};

void usage(const char *program)
{
    fprintf(stderr, "%s [-c | --config] <filename> <args>\n\n", program);
    fprintf(stderr, "arguments:\n");
    fprintf(stderr, "\t-?, -h\t\t\tShow this help.\n");
    fprintf(stderr, "\t--config <filename>\n");
    fprintf(stderr, "\t-c <filename>\t\tConfiguration file.\n");
    fprintf(stderr, "\t-v\t\t\tIncrease verbosity.\n");
    fprintf(stderr, "\t-q\t\t\tDecrease verbosity.\n");
}

void show_serial_ports(void)
{
    serial_t **ports = talloc_zero(NULL, serial_t *);
    size_t i;
    if (serial_list(&ports) != 0) {
        fprintf(stderr, "can't list serial ports: %s\n", strerror(errno));
        exit(1);
    }

    serial_t *port = NULL;
    for (i = 0; ports[i] != NULL; i++) {
        port = ports[i];
        switch (serial_transport(port)) {
        case SERIAL_TRANSPORT_NATIVE:
            dmr_log_info("serial port %s: native", serial_name(port));
            break;
        case SERIAL_TRANSPORT_USB: {
            int vid = 0, pid = 0;
            serial_usb_vid_pid(port, &vid, &pid);
            dmr_log_info("serial port %s: usb id %04X:%04X (%s)",
                serial_name(port), vid, pid, serial_usb_manufacturer(port));
            break;
        }
        case SERIAL_TRANSPORT_BLUETOOTH: {
            char *addr = NULL;
            if ((addr = serial_bluetooth_address(port))) {
                dmr_log_info("serial port %s: Bluetooth %s",
                    serial_name(port), addr);
            } else {
                dmr_log_info("serial port %s: Bluetooth 00:00:00:00:00:00",
                    serial_name(port));
            }
            talloc_free(addr);
            break;
        }
        default:
            dmr_log_info("serial port %s: unknown", serial_name(port));
            break;
        }
    }
}

int main(int argc, char **argv)
{
    int ch, ret = 0;
    char *filename = NULL;
    config_t *config = NULL;

    dmr_log_priority_set(DMR_LOG_PRIORITY_DEBUG);

    while ((ch = getopt_long(argc, argv, "c:h?vq", long_options, NULL)) != -1) {
        switch (ch) {
        case -1:       /* no more arguments */
        case 0:        /* long options toggles */
            break;
        case 'h':
        case '?':
            usage(argv[0]);
            return 0;
        case 'c':
            filename = strdup(optarg);
            break;
        case 'v':
            dmr_log_priority_set(dmr_log_priority() - 1);
            break;
        case 'q':
            dmr_log_priority_set(dmr_log_priority() + 1);
            break;
        default:
            exit(1);
            return 1;
        }
    }

    if (filename == NULL) {
        usage(argv[0]);
        return 1;
    }

    if ((ret = dmr_thread_name_set("main")) != 0) {
        dmr_log_error("noisebridge: can't set thread name: %s", strerror(ret));
    }

    show_serial_ports();

    if ((ret = init_config(filename)) != 0) {
        dmr_log_critical("noisebridge: init config failed with %d: %s",
            ret, dmr_error_get());
        exit(1);
        return 1;
    }

    if ((ret = init_script()) != 0) {
        dmr_log_critical("noisebridge: init script failed with %d: %s",
            ret, dmr_error_get());
        goto bail;
    }

    if ((ret = init_repeater()) != 0) {
        dmr_log_critical("noisebridge: init repeater failed with %d: %s",
            ret, dmr_error_get());
        goto bail;
    }

    if ((ret = loop_repeater()) != 0) {
        dmr_log_critical("noisebridge: loop repeater failed: %s", dmr_error_get());
    }

bail:
    if (config != NULL) {
        if (config->L != NULL) {
            lua_close(config->L);
        }
        talloc_free(config);
    }

    dmr_id_free();

    return ret;
}
