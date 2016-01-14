#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <talloc.h>
#include <dmr/config.h>
#include "config.h"
#include "util.h"
#if defined(HAVE_DL)
#include <dlfcn.h>
#endif
#if defined(HAVE_GETAUXVAL)
#include <sys/auxv.h>
#endif

const char *software_id = "NoiseBridge";
#ifdef PACKAGE_ID
const char *package_id = PACKAGE_ID;
#else
const char *package_id = "git:NoiseBridge";
#endif

static config_t *config = NULL;

const char *progname(void)
{
    const char *name = NULL;
#if defined(HAVE_GETAUXVAL)
    if ((name = (const char *)getauxval(AT_EXECFN)) != NULL)
        return name;
#endif
    return NULL;
}

plugin_t *load_plugin(char *filename, int argv, char **argc)
{
#if defined(HAVE_DL)
    plugin_t *plugin = talloc_zero(NULL, plugin_t);
    if (plugin == NULL) {
        dmr_error(DMR_ENOMEM);
        return NULL;
    }

    plugin->handle = dlopen(filename, RTLD_LAZY);
    char *error = NULL;
    if (plugin->handle == NULL) {
        dmr_log_error("noisebridge: can't open %s: %s", filename, dlerror());
        talloc_free(plugin);
        return NULL;
    }

    dlerror(); /* Clear any existing error */

    plugin->filename = talloc_strdup(plugin, filename);
    plugin->module = (module_t *)dlsym(plugin->handle, "module");
    if ((error = dlerror()) != NULL) {
        dmr_log_error("noisebridge: can't find entry point in %s", filename);
        talloc_free(plugin);
        return NULL;
    }

    dmr_log_info("noisebridge: loaded \"%s\" from %s", plugin->module->name, filename);
    return plugin;
#else
    dmr_log_error("noisebridge: module loading not supported");
    return NULL;
#endif
}

config_t *load_config(void)
{
    return config;
}

config_t *init_config(const char *filename)
{
    bool valid = true;
    FILE *fp;
    char *line = NULL, *k, *v;
    size_t len = 0;
    size_t lineno = 0;
    ssize_t read;

    if (config == NULL) {
        config = talloc_zero(NULL, config_t);
        if (config == NULL) {
            dmr_log_critical("noisebridge: out of memory");
            return NULL;
        }
    }

    config->filename = filename;
    config->log_level = DMR_LOG_PRIORITY_INFO;
    config->mbe_quality = 3;

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
            config->log_level = atoi(v);
            dmr_log_trace("noisebridge: config %s = %d", k, config->log_level);
            dmr_log_priority_set(config->log_level);

        } else if (!strcmp(k, "load")) {
            if (config->plugins >= NOISEBRIDGE_MAX_PLUGINS) {
                dmr_log_critical("noisebridge: %s[%zu]: max number of plugins reached", filename, lineno);
                valid = false;
                break;
            }
            char *args;
            v = strtok_r(v, ",", &args);
            trim(v);
            trim(args);
            plugin_t *plugin = load_plugin(v, strlen(args) > 0 ? 1 : 0, NULL);
            if (plugin == NULL) {
                dmr_log_critical("noisebridge: %s[%zu]: load failed", filename, lineno);
                valid = false;
                break;
            }

        } else if (!strcmp(k, "repeater_color_code")) {
            config->repeater_color_code = atoi(v);

        } else if (!strcmp(k, "repeater_route")) {
            config->repeater_route[config->repeater_routes] = route_rule_parse(v);
            if (config->repeater_route[config->repeater_routes] == NULL) {
                dmr_log_critical("noisebridge: %s[%zu]: invalid route rule", filename, lineno);
                valid = false;
                break;
            }
            config->repeater_routes++;

        } else if (!strcmp(k, "upstream_type")) {
            if (!strcmp(v, "homebrew")) {
                config->upstream = PEER_HOMEBREW;
            } else {
                dmr_log_critical("noisebridge: %s[%zu]: unsupported type %s", filename, lineno, v);
                valid = false;
                break;
            }

        } else if (!strcmp(k, "homebrew_host")) {
            config->homebrew_host = gethostbyname(v);
            if (config->homebrew_host == NULL) {
                dmr_log_critical("noisebridge: %s[%zu]: unresolved name %s", filename, lineno, v);
                valid = false;
                break;
            }
            config->homebrew_host_s = strdup(v);

        } else if (!strcmp(k, "homebrew_port")) {
            config->homebrew_port = atoi(v);

        } else if (!strcmp(k, "homebrew_auth")) {
            config->homebrew_auth = talloc_strdup(config, v);

        } else if (!strcmp(k, "homebrew_id")) {
            config->homebrew_id = (dmr_id_t)atoi(v);
            dmr_homebrew_config_repeater_id(&config->homebrew_config, config->homebrew_id);

        } else if (!strcmp(k, "homebrew_call")) {
            dmr_homebrew_config_callsign(&config->homebrew_config, v);

        } else if (!strcmp(k, "homebrew_cc")) {
            dmr_homebrew_config_color_code(&config->homebrew_config, (dmr_id_t)atoi(v));

        } else if (!strcmp(k, "homebrew_rx_freq")) {
            dmr_homebrew_config_rx_freq(&config->homebrew_config, strtol(v, NULL, 10));

        } else if (!strcmp(k, "homebrew_tx_freq")) {
            dmr_homebrew_config_tx_freq(&config->homebrew_config, strtol(v, NULL, 10));

        } else if (!strcmp(k, "homebrew_tx_power")) {
            dmr_homebrew_config_tx_power(&config->homebrew_config, atoi(v));

        } else if (!strcmp(k, "homebrew_height")) {
            dmr_homebrew_config_height(&config->homebrew_config, atoi(v));

        } else if (!strcmp(k, "homebrew_latitude")) {
            dmr_homebrew_config_latitude(&config->homebrew_config, strtod(v, NULL));

        } else if (!strcmp(k, "homebrew_longitude")) {
            dmr_homebrew_config_longitude(&config->homebrew_config, strtod(v, NULL));

        } else if (!strcmp(k, "modem_type")) {
            if (!strcmp(v, "mmdvm")) {
                config->modem = PEER_MMDVM;

            } else if (!strcmp(v, "mbe")) {
                config->modem = PEER_MBE;
                config->audio_needed = true;

            } else {
                dmr_log_critical("noisebridge: %s[%zu]: unsupported type %s", filename, lineno, v);
                valid = false;
                break;
            }

        } else if (!strcmp(k, "mbe_quality")) {
            config->mbe_quality = atoi(v);

        } else if (!strcmp(k, "mmdvm_port")) {
            config->mmdvm_port = talloc_strdup(config, v);

        } else if (!strcmp(k, "mmdvm_rate")) {
            config->mmdvm_rate = atoi(v);

        } else if (!strcmp(k, "audio_device")) {
            config->audio_device = talloc_strdup(config, v);

        } else {
            dmr_log_critical("noisebridge: %s[%zu]: syntax error, invalid key \"%s\"", filename, lineno, k);
            valid = false;
            break;
        }
    }
    fclose(fp);

    if (!valid)
        return NULL;

    return config;
}

void kill_config(void)
{
    if (config->homebrew != NULL) {
        dmr_homebrew_free(config->homebrew);
    }
#if defined(DMR_ENABLE_PROTO_MBE)
    if (config->mbe != NULL) {
        dmr_mbe_free(config->mbe);
    }
#endif
}
