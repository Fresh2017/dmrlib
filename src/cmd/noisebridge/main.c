#include <getopt.h>
#include <stdio.h>
#include <stdarg.h>
#include <dmr.h>
#include <dmr/packet.h>
#include <dmr/proto/repeater.h>
#include "config.h"
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

    if (init_config(filename) != 0) {
        exit(1);
        return 1;
    }

    if (init_script() != 0) {
        ret = 1;
        goto bail;
    }

    if (init_repeater() != 0) {
        ret = 1;
        goto bail;
    }

    ret = loop_repeater();

bail:
    if (config != NULL) {
        if (config->L != NULL) {
            lua_close(config->L);
        }
        talloc_free(config);
    }

    return ret;
}
