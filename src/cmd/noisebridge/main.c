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

bool init_dmr(config_t *config)
{
    bool valid = true;

    switch (config->modem) {
    case PEER_MBE:
        dmr_log_info("noisebridge: MBE decoder with quality %d", config->mbe_quality);
        config->mbe = dmr_mbe_new(config->mbe_quality);
        if (config->mbe == NULL) {
            valid = false;
            break;
        }
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
        config->mmdvm = dmr_mmdvm_open(config->mmdvm_port, config->mmdvm_rate, 1000UL);
        if (config->mmdvm == NULL) {
            valid = false;
            break;
        }
        /*
        if (!dmr_mmdvm_sync(config->mmdvm)) {
            valid = false;
            dmr_log_critical("noisebridge: %s: mmdvm modem sync failed\n", config->filename);
            return NULL;
        }
        */
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
        if (!(valid = (config->homebrew_call != NULL))) {
            dmr_log_critical("noisebridge: %s: homebrew_call required", config->filename);
            break;
        }
        if (!(valid = (config->homebrew_id != 0))) {
            dmr_log_critical("noisebridge: %s: homebrew_id required", config->filename);
            break;
        }
        if (!(valid = (config->homebrew_cc >= 1 || config->homebrew_cc <= 15))) {
            dmr_log_critical("noisebridge: %s: homebrew_cc required (1 >= cc >= 15)", config->filename);
            break;
        }


        struct in_addr **addr_list;
        struct in_addr server_addr;
        int i;
        addr_list = (struct in_addr **)config->homebrew_host->h_addr_list;
        for (i = 0; addr_list[i] != NULL; i++) {
            server_addr.s_addr = (*addr_list[0]).s_addr;
            dmr_log_debug("noisebridge: %s resolved to %s",
                config->homebrew_host->h_name,
                inet_ntoa(server_addr));

            /*
            printf("noisebridge: connecting to HomeBrew repeater at %s:%d on %s\n",
                inet_ntoa(server_addr), config->homebrew_port,
                inet_ntoa(config->homebrew_bind));
            */
            config->homebrew = dmr_homebrew_new(
                config->homebrew_bind,
                config->homebrew_port,
                server_addr);
            dmr_homebrew_config_callsign(config->homebrew->config, config->homebrew_call);
            dmr_homebrew_config_repeater_id(config->homebrew->config, config->homebrew_id);
            dmr_homebrew_config_color_code(config->homebrew->config, config->homebrew_cc);
            dmr_homebrew_config_software_id(config->homebrew->config, software_id);
            dmr_homebrew_config_package_id(config->homebrew->config, package_id);
            /*
            if (config->mmdvm != NULL && config->mmdvm->firmware != NULL) {
                dmr_homebrew_config_software_id(config->homebrew->config, config->mmdvm->firmware);
                bool zero = false;
                for (j = 0; j < sizeof(config->homebrew->config->software_id); j++) {
                    if (zero) {
                        config->homebrew->config->software_id[j] = 0x20;
                    } else if (config->homebrew->config->software_id[j] == '(') {
                        config->homebrew->config->software_id[j] = 0x20;
                        config->homebrew->config->software_id[j - 1] = 0x20;
                        zero = true;
                    } else if (config->homebrew->config->software_id[j] == ' ') {
                        config->homebrew->config->software_id[j] = '-';
                    }
                }
            }
            */
            if (dmr_log_priority() <= DMR_LOG_PRIORITY_DEBUG) {
                dmr_log_debug("noisebridge: dumping homebrew config struct:");
                dump_hex(config->homebrew->config, sizeof(dmr_homebrew_config_t));
            }
            if (!(valid = (dmr_homebrew_auth(config->homebrew, config->homebrew_auth) == 0)))
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

bool init_repeater(config_t *config)
{
    dmr_repeater_t *repeater = dmr_repeater_new();
    if (repeater == NULL)
        return false;

    if (config->homebrew != NULL) {
        dmr_log_info("noisebridge: starting homebrew proto");
        if (!dmr_proto_init(config->homebrew))
            return false;
        if (!dmr_proto_start(config->homebrew))
            return false;
        dmr_repeater_add(repeater, config->homebrew, &config->homebrew->proto);
    }
    if (config->mmdvm != NULL) {
        dmr_log_info("noisebridge: starting mmdvm proto");
        if (!dmr_proto_init(config->mmdvm))
            return false;
        if (!dmr_proto_start(config->mmdvm))
            return false;
        dmr_repeater_add(repeater, config->mmdvm, &config->mmdvm->proto);
    }
    if (config->mbe != NULL) {
        dmr_log_info("noisebridge: starting mbe proto");
        if (!dmr_proto_init(config->mbe))
            return false;
        if (!dmr_proto_start(config->mbe))
            return false;
        dmr_repeater_add(repeater, config->mbe, &config->mbe->proto);
    }

    if (!dmr_proto_init(repeater))
        return false;

    return dmr_proto_start(repeater);
}

int main(int argc, char **argv)
{
    config_t *config = NULL;
    int status = 0;

    dmr_log_priority_set(DMR_LOG_PRIORITY_VERBOSE);
    if (argc != 2) {
        fprintf(stderr, "%s <config>\n", argv[0]);
        status = 1;
        goto bail;
    }

    config = configure(argv[1]);
    if (config == NULL) {
        status = 1;
        goto bail;
    }

    dmr_log_priority_set(config->log_level);

    if (!init_audio(config)) {
        status = 1;
        goto bail;
    }

    if (!boot_audio(config)) {
        status = 1;
        goto bail;
    }

    if (!init_dmr(config)) {
        status = 1;
        goto bail;
    }

    if (!init_repeater(config)) {
        status = 1;
        goto bail;
    }

bail:
    if (config != NULL) {
        if (config->homebrew != NULL) {
            dmr_homebrew_free(config->homebrew);
        }
        if (config->mbe != NULL) {
            dmr_mbe_free(config->mbe);
        }
    }

    return status;
}
