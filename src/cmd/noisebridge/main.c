#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <portaudio.h>

#include "config.h"
#include "audio.h"
#include "http.h"
#include "repeater.h"

int main(int argc, char **argv)
{
    config_t *config = NULL;
    int status = 0, ret;

    dmr_log_priority_set(DMR_LOG_PRIORITY_DEBUG);
    if ((ret = dmr_thread_name_set("main")) != 0) {
        dmr_log_error("noisebridge: can't set thread name: %s", strerror(ret));
    }
    if (argc < 2) {
        fprintf(stderr, "%s <config>\n", argv[0]);
        status = 1;
        goto bail;
    }

    if (init_config(argv[1]) == NULL) {
        status = 1;
        goto bail;
    }

    config = load_config();
    dmr_log_priority_set(config->log_level);

    if (config->audio_needed) {
        if (!init_audio()) {
            status = 1;
            goto bail;
        }
        if (!boot_audio()) {
            status = 1;
            goto bail;
        }
    }

    if (!init_http()) {
        status = 1;
        goto bail;
    }

    if (!boot_http()) {
        status = 1;
        goto bail;   
    }

    if (!init_repeater()) {
        status = 1;
        goto bail;
    }

    if (!loop_repeater()) {
        status = 1;
        goto bail;
    }

bail:
    kill_config();

    if (config->audio_needed)
        kill_audio();

    return status;
}
