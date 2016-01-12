#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include "config.h"
#include "util.h"

const char *software_id = "NoiseBridge";
#ifdef PACKAGE_ID
const char *package_id = PACKAGE_ID;
#else
const char *package_id = "NoiseBridge-git";
#endif

static config_t config;

config_t *load_config(void)
{
    return &config;
}

config_t *init_config(const char *filename)
{
    bool valid = true;
    FILE *fp;
    char *line = NULL, *k, *v;
    size_t len = 0;
    size_t lineno = 0;
    ssize_t read;

    memset(&config, 0, sizeof(config_t));
    config.filename = filename;
    config.log_level = DMR_LOG_PRIORITY_INFO;
    config.mbe_quality = 3;

    if ((fp = fopen(filename, "r")) == NULL) {
        dmr_log_critical("failed to open %s: %s", filename, strerror(errno));
        return NULL;
    }

    while ((read = getline(&line, &len, fp)) != -1) {
        lineno++;
        trim(line);
        dmr_log_trace("noisebridge: %s[%zu]: %s", filename, lineno, line);

        if (strlen(line) == 0 || line[0] == '#' || line[0] == ';')
            continue;

        k = strtok_r(line, "=", &v);
        trim(k);
        trim(v);
        if (k == NULL || v == NULL || strlen(v) == 0) {
            dmr_log_critical("noisebridge: %s[%zu]: syntax error", filename, lineno);
            valid = false;
            break;
        }

        dmr_log_debug("noisebridge: config %s = \"%s\"", k, v);
        if (!strcmp(k, "log_level")) {
            config.log_level = atoi(v);
            dmr_log_trace("noisebridge: config %s = %d", k, config.log_level);
            dmr_log_priority_set(config.log_level);

        } else if (!strcmp(k, "repeater_color_code")) {
            config.repeater_color_code = atoi(v);

        } else if (!strcmp(k, "repeater_route")) {
            config.repeater_route[config.repeater_routes] = route_rule_parse(v);
            if (config.repeater_route[config.repeater_routes] == NULL) {
                dmr_log_critical("noisebridge: %s[%zu]: invalid route rule", filename, lineno);
                valid = false;
                break;
            }
            config.repeater_routes++;

        } else if (!strcmp(k, "upstream_type")) {
            if (!strcmp(v, "homebrew")) {
                config.upstream = PEER_HOMEBREW;
            } else {
                dmr_log_critical("noisebridge: %s[%zu]: unsupported type %s", filename, lineno, v);
                valid = false;
                break;
            }

        } else if (!strcmp(k, "homebrew_host")) {
            config.homebrew_host = gethostbyname(v);
            if (config.homebrew_host == NULL) {
                dmr_log_critical("noisebridge: %s[%zu]: unresolved name %s", filename, lineno, v);
                valid = false;
                break;
            }
            config.homebrew_host_s = strdup(v);

        } else if (!strcmp(k, "homebrew_port")) {
            config.homebrew_port = atoi(v);

        } else if (!strcmp(k, "homebrew_auth")) {
            config.homebrew_auth = strdup(v);

        } else if (!strcmp(k, "homebrew_call")) {
            config.homebrew_call = strdup(v);

        } else if (!strcmp(k, "homebrew_id")) {
            config.homebrew_id = (dmr_id_t)atoi(v);

        } else if (!strcmp(k, "homebrew_cc")) {
            config.homebrew_cc = (dmr_id_t)atoi(v);

        } else if (!strcmp(k, "homebrew_tx_freq")) {
            config.homebrew_tx_freq = strtol(v, NULL, 10);

        } else if (!strcmp(k, "homebrew_rx_freq")) {
            config.homebrew_rx_freq = strtol(v, NULL, 10);

        } else if (!strcmp(k, "homebrew_latitude")) {
            config.homebrew_latitude = strtod(v, NULL);

        } else if (!strcmp(k, "homebrew_longitude")) {
            config.homebrew_longitude = strtod(v, NULL);

        } else if (!strcmp(k, "modem_type")) {
            if (!strcmp(v, "mmdvm")) {
                config.modem = PEER_MMDVM;

            } else if (!strcmp(v, "mbe")) {
                config.modem = PEER_MBE;

            } else {
                dmr_log_critical("noisebridge: %s[%zu]: unsupported type %s", filename, lineno, v);
                valid = false;
                break;
            }

        } else if (!strcmp(k, "mbe_quality")) {
            config.mbe_quality = atoi(v);

        } else if (!strcmp(k, "mmdvm_port")) {
            config.mmdvm_port = strdup(v);

        } else if (!strcmp(k, "mmdvm_rate")) {
            config.mmdvm_rate = atoi(v);

        } else if (!strcmp(k, "audio_device")) {
            config.audio_device = strdup(v);

        } else {
            dmr_log_critical("noisebridge: %s[%zu]: syntax error, invalid key \"%s\"", filename, lineno, k);
            valid = false;
            break;
        }
    }
    fclose(fp);

    if (!valid)
        return NULL;

    return &config;
}

void kill_config(void)
{
    if (config.homebrew != NULL) {
        dmr_homebrew_free(config.homebrew);
    }
    if (config.mbe != NULL) {
        dmr_mbe_free(config.mbe);
    }
}
