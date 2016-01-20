#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <signal.h>

#include <dmr/config.h>
#include <dmr/error.h>
#include <dmr/proto.h>

#include "audio.h"
#include "repeater.h"

static volatile bool active = true;
static dmr_repeater_t *repeater = NULL;

void kill_handler(int sig)
{
    DMR_UNUSED(sig);
    if (active) {
        dmr_log_info("repeater: interrupt, stopping, hit again to kill");
        active = false;
        if (repeater != NULL) {
            dmr_proto_call_stop(repeater);
        }
    } else {
        dmr_log_info("repeater: interrupt, killing");
        exit(2);
    }
}

bool init_repeater()
{
    bool valid = true;
    config_t *config = load_config();

    if (config->repeater_routes == 0) {
        dmr_log_warn("noisebrige: no route rules defined, repeater will be useless!");
    }

    switch (config->modem) {
    case PEER_MBE:
#if !defined(WITH_MBELIB)
        dmr_log_critical("noisebridge: libdmr not compiled with MBE support");
        valid = false;
#else
        dmr_log_info("noisebridge: MBE decoder with quality %d", config->mbe_quality);
        config->mbe = dmr_mbe_new(config->mbe_quality, stream_audio);
        if (config->mbe == NULL) {
            valid = false;
            break;
        }
#endif
        break;

    case PEER_MMDVM:
        if (!(valid = (config->mmdvm_port != NULL))) {
            dmr_log_critical("noisebridge: %s: mmdvm_port required", config->filename);
            break;
        }
        if (config->mmdvm_rate == 0) {
            config->mmdvm_rate = DMR_MMDVM_BAUD_RATE;
        }

        dmr_log_info("noisebridge: connecting to MMDVM modem on port %s with %d baud",
            config->mmdvm_port, config->mmdvm_rate);

        uint16_t flag = DMR_MMDVM_FLAG_FIXUP_VOICE |
                        DMR_MMDVM_FLAG_FIXUP_VOICE_LC;
        config->mmdvm = dmr_mmdvm_open(config->mmdvm_port, config->mmdvm_rate, flag);
        if (config->mmdvm == NULL) {
            exit(1);
            break;
        }

        if ((flag & DMR_MMDVM_FLAG_SYNC) > 0 && dmr_mmdvm_sync(config->mmdvm) != 0) {
            valid = false;
            dmr_log_critical("noisebridge: %s: mmdvm modem sync failed\n", config->filename);
        }
        break;

    default:
        valid = false;
        dmr_log_critical("noisebridge: %s: modem_type required", config->filename);
        break;
    }

    switch (config->upstream) {
    case PEER_HOMEBREW:
        if (!(valid = (config->homebrew_host != NULL))) {
            dmr_log_critical("noisebridge: %s: homebrew_host required", config->filename);
            break;
        }
        if (config->homebrew_port == 0) {
            config->homebrew_port = DMR_HOMEBREW_PORT;
        }
        if (!(valid = (config->homebrew_auth != NULL))) {
            dmr_log_critical("noisebridge: %s: homebrew_auth required", config->filename);
            break;
        }
        if (!(valid = (config->homebrew_id != 0))) {
            dmr_log_critical("noisebridge: %s: homebrew_id required", config->filename);
            break;
        }

        struct in_addr **addr_list;
        struct in_addr server_addr;
        int i;
        addr_list = (struct in_addr **)config->homebrew_host->h_addr_list;
        for (i = 0; addr_list[i] != NULL; i++) {
            server_addr.s_addr = (*addr_list[0]).s_addr;
            dmr_log_debug("noisebridge: connecting to homebrew system %s(%s) as %d",
                config->homebrew_host->h_name,
                inet_ntoa(server_addr),
                config->homebrew_id);

            config->homebrew = dmr_homebrew_new(
                config->homebrew_port,
                server_addr);
            memcpy(&config->homebrew->config, &config->homebrew_config, sizeof(dmr_homebrew_config_t));
            config->homebrew->id = config->homebrew_id;
            valid = (dmr_homebrew_auth(config->homebrew, config->homebrew_auth) == 0);
            if (valid)
                break;

            free(config->homebrew);
            config->homebrew = NULL;
        }
        break;

    default:
        valid = false;
        dmr_log_critical("noisebridge: %s: homebrew_type required", config->filename);
        break;
    }

    return valid;
}

static int loop_repeater_start_protos(void)
{
    int ret;
    config_t *config = load_config();

    if ((ret = dmr_proto_init()) != 0) {
        dmr_log_critical("noisebridge: can't init proto");
        return ret;
    }
    if (config->homebrew != NULL) {
        dmr_log_info("noisebridge: starting homebrew proto");
        if ((ret = dmr_proto_call_init(config->homebrew)) != 0)
            return ret;
        if ((ret = dmr_proto_call_start(config->homebrew)) != 0)
            return ret;
    } else {
        dmr_log_info("noisebridge: homebrew not enabled");
    }
    if (config->mmdvm != NULL) {
        dmr_log_info("noisebridge: starting mmdvm proto");
        if ((ret = dmr_proto_call_init(config->mmdvm)) != 0)
            return ret;
        if ((ret = dmr_proto_call_start(config->mmdvm)) != 0)
            return ret;

        if (config->mmdvm_rx_freq || config->mmdvm_tx_freq) {
            dmr_msleep(250);
            dmr_mmdvm_set_rf_config(config->mmdvm,
                config->mmdvm_rx_freq, config->mmdvm_tx_freq);
        }
    } else {
        dmr_log_info("noisebridge: mmdvm not enabled");
    }
    if (config->mbe != NULL) {
        dmr_log_info("noisebridge: starting mbe proto");
        if ((ret = dmr_proto_call_init(config->mbe)) != 0)
            return ret;
        if ((ret = dmr_proto_call_start(config->mbe)) != 0)
            return ret;
    } else {
        dmr_log_info("noisebridge: mbe not enabled");
    }

    return 0;
}

static void loop_repeater_stop_protos(void)
{
    config_t *config = load_config();
    if (config->homebrew && dmr_proto_is_active(config->homebrew)) {
        dmr_log_info("noisebridge: stopping homebrew proto");
        dmr_proto_call_stop(config->homebrew);
    }
    if (config->mmdvm && dmr_proto_is_active(config->mmdvm)) {
        dmr_log_info("noisebridge: stopping mmdvm proto");
        dmr_proto_call_stop(config->mmdvm);
    }
    if (config->mbe && dmr_proto_is_active(config->mbe)) {
        dmr_log_info("noisebridge: stopping mbe proto");
        dmr_proto_call_stop(config->mbe);
    }
}

bool loop_repeater(void)
{
    bool ret = false;
    config_t *config = load_config();
    repeater = dmr_repeater_new(route_rule_packet);
    if (repeater == NULL)
        return false;
    repeater->color_code = config->repeater_color_code;

    if (loop_repeater_start_protos() != 0)
        goto bail_loop_repeater;

    if (config->homebrew != NULL &&
        dmr_repeater_add(repeater, config->homebrew, &config->homebrew->proto) != 0)
        goto bail_loop_repeater;
    if (config->mmdvm != NULL &&
        dmr_repeater_add(repeater, config->mmdvm, &config->mmdvm->proto) != 0)
        goto bail_loop_repeater;
    if (config->mbe != NULL &&
        dmr_repeater_add(repeater, config->mbe, &config->mbe->proto) != 0)
        goto bail_loop_repeater;

    if (dmr_proto_call_init(repeater) != 0)
        goto bail_loop_repeater;

    dmr_log_info("noisebridge: starting repeater proto");
    if ((ret = dmr_proto_call_start(repeater)) != 0)
        dmr_log_critical("noisebridge: start of repeater failed");

    active = true;
    signal(SIGINT, kill_handler);

    dmr_log_info("noisebridge: running repeater");
    while (active) {
        dmr_msleep(60);
    }
    if ((ret = dmr_proto_call_wait(repeater)) != 0)
        goto bail_loop_repeater;
    goto stop_loop_repeater;

bail_loop_repeater:
    dmr_log_error("noisebridge: repeater stopped with error: %s", dmr_error_get());

stop_loop_repeater:
    loop_repeater_stop_protos();
    dmr_repeater_free(repeater);
    return ret;
}
