#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include "config.h"

const char *software_id = "NoiseBridge";
#ifdef PACKAGE_ID
const char *package_id = PACKAGE_ID;
#else
const char *package_id = "NoiseBridge-git";
#endif

void rtrim(char *str)
{
    size_t n;
    n = strlen(str);
    while (n > 0 && isspace((unsigned char)str[n - 1])) {
        n--;
    }
    str[n] = '\0';
}

void ltrim(char *str)
{
    size_t n;
    n = 0;
    while (str[n] != '\0' && isspace((unsigned char)str[n])) {
        n++;
    }
    memmove(str, str + n, strlen(str) - n + 1);
}

void trim(char *str)
{
    rtrim(str);
    ltrim(str);
}

config_t *configure(const char *filename)
{
    static config_t config;
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
        dmr_log_verbose("noisebridge: %s[%zu]: %s", filename, lineno, line);

        if (strlen(line) == 0 || line[0] == '#' || line[0] == ';')
            continue;

        k = strtok_r(line, "=", &v);
        trim(k);
        trim(v);
        if (k == NULL || v == NULL || strlen(v) == 0) {
            dmr_log_critical("noisebridge: %s[%zu]: syntax error\n", filename, lineno);
            valid = false;
            break;
        }

        dmr_log_debug("noisebridge: config %s = \"%s\"", k, v);
        if (!strcmp(k, "log_level")) {
            config.log_level = atoi(v);
            dmr_log_verbose("noisebridge: config %s = %d", k, config.log_level);

        } else if (!strcmp(k, "upstream_type")) {
            if (!strcmp(v, "homebrew")) {
                config.upstream = PEER_HOMEBREW;
            } else {
                dmr_log_critical("noisebridge: %s[%zu]: unsupported type %s\n", filename, lineno, v);
                valid = false;
                break;
            }

        } else if (!strcmp(k, "homebrew_host")) {
            config.homebrew_host = gethostbyname(v);
            if (config.homebrew_host == NULL) {
                dmr_log_critical("noisebridge: %s[%zu]: unresolved name %s\n", filename, lineno, v);
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

        } else if (!strcmp(k, "modem_type")) {
            if (!strcmp(v, "mmdvm")) {
                config.modem = PEER_MMDVM;

            } else if (!strcmp(v, "mbe")) {
                config.modem = PEER_MBE;

            } else {
                dmr_log_critical("noisebridge: %s[%zu]: unsupported type %s\n", filename, lineno, v);
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
