#include <ctype.h>
#include <stdlib.h>
#include <talloc.h>
#include <string.h>
#include <arpa/inet.h>
#include <dmr/config.h>
#include <dmr/id.h>
#include <dmr/error.h>
#include <dmr/malloc.h>
#include "config.h"
#include "common/byte.h"
#include "common/format.h"
#include "common/scan.h"
#include "common/serial.h"

#if defined(DMR_HAVE_NETDB_H)
#include <netdb.h>
#endif
#if defined(DMR_PLATFORM_WINDOWS)
#include <winsock2.h>
#endif

static config_t *config = NULL;

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

#define CONFIG_ERROR(x,...) do { \
    dmr_log_error("noisebridge: %s[%zu]: " x, filename, lineno, ##__VA_ARGS__); \
    return dmr_error(DMR_EINVAL); \
} while(0)

#define CONFIG_STR(_b, _m, _t) \
if (!strcmp(k, _m)) { \
    if (_t != NULL) { CONFIG_ERROR("duplicate key %s", k); } \
    _t = talloc_strdup(_b, v); \
}

#define CONFIG_INT(_b, _m, _t) \
if (!strcmp(k, _m)) { \
    if (_t != 0) { CONFIG_ERROR("duplicate key %s", k); } \
    _t = atoi(v); \
}

#define CONFIG_FLOAT(_b, _m, _t) \
if (!strcmp(k, _m)) { \
    if (_t != 0) { CONFIG_ERROR("duplicate key %s", k); } \
    _t = atof(v); \
}

bool split(char *line, char *sep, char **k, char **v)
{
    *k = strtok_r(line, sep, v);
    if (*k != NULL) {
        trim(*k);
    }
    if (*v != NULL) {
        trim(*v);
    }
    return *k != NULL && *v != NULL && strlen(*v) > 0;
}

int read_config(char *line, char *filename, size_t lineno);

int read_config_repeater(char *line, char *filename, size_t lineno)
{
    dmr_log_debug("noisebridge: %s[%zu]: (repeater) %s", filename, lineno, line);
    if (strlen(line) == 0 || line[0] == '#' || line[0] == ';') {
        return 0;
    }
    if (!strcmp(line, "}")) {
        dmr_log_debug("noisebridge: %s[%zu]: end of section repeater", filename, lineno);
        config->section = read_config;
        return 0;
    }

    char *k = NULL, *v = NULL;
    if (!split(line, "=", &k, &v)) {
        CONFIG_ERROR("syntax error \"%s\"", line);
    }
    dmr_log_debug("noisebridge: config %s = \"%s\"", k, v);

    if (!strcmp(k, "script")) {
        if (config->repeater.script != NULL) {
            dmr_log_critical("noisebridge: %s[%zu]: duplicate script", filename, lineno);
            return -1;
        }
        config->repeater.script = talloc_strdup(config, v);
    } else if (!strcmp(k, "timeout")) {
        config->repeater.timeout = atoi(v);
    } else {
        CONFIG_ERROR("unknown key \"%s\"", k);
    }
    return 0;
}

int read_config_dmrid(char *line, char *filename, size_t lineno)
{
    dmr_log_debug("noisebridge: %s[%zu]: (dmrid) %s", filename, lineno, line);
    if (strlen(line) == 0 || line[0] == '#' || line[0] == ';') {
        return 0;
    }
    if (!strcmp(line, "}")) {
        dmr_log_debug("noisebridge: %s[%zu]: end of section dmrid", filename, lineno);
        config->section = read_config;
        return 0;
    }

    char *k = NULL, *v = NULL;
    if (!split(line, "=", &k, &v)) {
        CONFIG_ERROR("syntax error \"%s\"", line);
    }
    dmr_log_debug("noisebridge: config %s = \"%s\"", k, v);

    dmr_id id;
    if ((id = atoi(k)) == 0) {
        CONFIG_ERROR("invalid DMR ID \"%s\"", k);
    }

    return dmr_id_add(id, v);
}

int read_config_httpd(char *line, char *filename, size_t lineno)
{
    dmr_log_debug("noisebridge: %s[%zu]: (httpd) %s", filename, lineno, line);
    if (strlen(line) == 0 || line[0] == '#' || line[0] == ';') {
        return 0;
    }
    if (!strcmp(line, "}")) {
        dmr_log_debug("noisebridge: %s[%zu]: end of section httpd", filename, lineno);
        if (config->httpd.port == 0) {
            config->httpd.port = 8042;
        }
        config->section = read_config;
        return 0;
    }

    char *k = NULL, *v = NULL;
    if (!split(line, "=", &k, &v)) {
        CONFIG_ERROR("syntax error \"%s\"", line);
    }
    dmr_log_debug("noisebridge: config %s = \"%s\"", k, v);

    if (!strcmp(k, "bind")) {
        if ((scan_ip6(v, config->httpd.bind)) == 0) {
            CONFIG_ERROR("invalid IP address");
        }
    } else if (!strcmp(k, "port")) {
        config->httpd.port = atoi(v);
    } else if (!strcmp(k, "root")) {
        format_path_canonical(config->httpd.root, PATH_MAX, v);
        if (strlen(config->httpd.root) == 0) {
            CONFIG_ERROR("unknown path \"%s\"", v);
        }
        dmr_log_debug("config: httpd root resolved to \"%s\"", config->httpd.root);
    } else {
        CONFIG_ERROR("unknown key \"%s\"", k);
    }
    return 0;
}

int read_config_homebrew(char *line, char *filename, size_t lineno)
{
    proto_t *proto = config->proto[config->protos - 1];
    dmr_log_debug("noisebridge: %s[%zu]: (homebrew) %s", filename, lineno, line);
    if (strlen(line) == 0 || line[0] == '#' || line[0] == ';') {
        return 0;
    }
    if (!strcmp(line, "}")) {
        dmr_log_debug("noisebridge: %s[%zu]: end of section homebrew", filename, lineno);
        if (byte_equal(proto->settings.homebrew.peer_ip, ip6any, 16)) {
            CONFIG_ERROR("no peer configured");
        }
        config->section = read_config;
        return 0;
    }

    char *k = NULL, *v = NULL;
    if (!split(line, "=", &k, &v)) {
        CONFIG_ERROR("syntax error \"%s\"", line);
    }
    dmr_log_debug("noisebridge: config %s = \"%s\"", k, v);

    CONFIG_STR(proto, "name", proto->name)
    else CONFIG_STR(proto, "auth", proto->settings.homebrew.auth)
    else CONFIG_STR(proto, "call", proto->settings.homebrew.call)
    else CONFIG_INT(proto, "repeater_id", proto->settings.homebrew.repeater_id)
    else CONFIG_INT(proto, "color_code", proto->settings.homebrew.color_code)
    else CONFIG_INT(proto, "rx_freq", proto->settings.homebrew.rx_freq)
    else CONFIG_INT(proto, "tx_freq", proto->settings.homebrew.tx_freq)
    else CONFIG_INT(proto, "tx_power", proto->settings.homebrew.tx_power)
    else CONFIG_INT(proto, "height", proto->settings.homebrew.height)
    else CONFIG_FLOAT(proto, "latitude", proto->settings.homebrew.latitude)
    else CONFIG_FLOAT(proto, "longitude", proto->settings.homebrew.longitude)
    else CONFIG_INT(proto, "port", proto->settings.homebrew.peer_port)
    else CONFIG_INT(proto, "bind_port", proto->settings.homebrew.bind_port)
    else if (!strcmp(k, "host")) {
        int ret;
        if ((ret = ip6resolve(proto->settings.homebrew.peer_ip, v)) != 0) {
            dmr_log_critical("failed to resolve %s: %s", v, gai_strerror(errno));
            return ret;
        }
    } else if (!strcmp(k, "bind_host")) {
        int ret;
        if ((ret = ip6resolve(proto->settings.homebrew.bind_ip, v)) != 0) {
            dmr_log_critical("failed to resolve %s: %s", v, gai_strerror(errno));
            return ret;
        }
    } else {
        CONFIG_ERROR("unknown key \"%s\"", k);
    }
    return 0;
}

int read_config_mbe(char *line, char *filename, size_t lineno)
{
    proto_t *proto = config->proto[config->protos - 1];
    dmr_log_debug("noisebridge: %s[%zu]: (mbe) %s", filename, lineno, line);
    if (strlen(line) == 0 || line[0] == '#' || line[0] == ';') {
        return 0;
    }
    if (!strcmp(line, "}")) {
        dmr_log_debug("noisebridge: %s[%zu]: end of section mbe", filename, lineno);
        config->section = read_config;
        return 0;
    }

    char *k = NULL, *v = NULL;
    if (!split(line, "=", &k, &v)) {
        CONFIG_ERROR("syntax error \"%s\"", line);
        return -1;
    }
    dmr_log_debug("noisebridge: config %s = \"%s\"", k, v);
    CONFIG_STR(proto, "name", proto->name)
    else CONFIG_INT(proto, "quality", proto->settings.mbe.quality)
    else {
        CONFIG_ERROR("unknown key \"%s\"", k);
    }
    return 0;
}

int read_config_mmdvm(char *line, char *filename, size_t lineno)
{
    proto_t *proto = config->proto[config->protos - 1];
    dmr_log_debug("noisebridge: %s[%zu]: (mmdvm) %s", filename, lineno, line);
    if (strlen(line) == 0 || line[0] == '#' || line[0] == ';') {
        return 0;
    }
    if (!strcmp(line, "}")) {
        dmr_log_debug("noisebridge: %s[%zu]: end of section mmdvm", filename, lineno);
        config->section = read_config;
        return 0;
    }

    char *k = NULL, *v = NULL;
    if (!split(line, "=", &k, &v)) {
        CONFIG_ERROR("syntax error \"%s\"", line);
    }
    dmr_log_debug("noisebridge: config %s = \"%s\"", k, v);
    CONFIG_STR(proto, "name", proto->name)
    else CONFIG_INT(proto, "color_code", proto->settings.mmdvm.color_code)
    else CONFIG_INT(proto, "rx_freq", proto->settings.mmdvm.rx_freq)
    else CONFIG_INT(proto, "tx_freq", proto->settings.mmdvm.tx_freq)
    else if (!strcmp(k, "port")) {
        if (v[0] == '/' || !strcmp(v, "COM")) {
            proto->settings.mmdvm.port = talloc_strdup(proto, v);
        } else {
            if (serial_find(v, &proto->settings.mmdvm.port) != 0)
                CONFIG_ERROR("error finding port %s: %s", v, strerror(errno));
        }
    } else if (!strcmp(k, "model")) {
        if (!strcmp(v, "generic")) {
            proto->settings.mmdvm.model = DMR_MMDVM_MODEL_G4KLX;
        } else if (!strcmp(v, "g4klx")) {
            proto->settings.mmdvm.model = DMR_MMDVM_MODEL_G4KLX;
        } else if (!strcmp(v, "dvmega")) {
            proto->settings.mmdvm.model = DMR_MMDVM_MODEL_DVMEGA;
        } else {
            CONFIG_ERROR("unknown MMDVM model \"%s\"", v);
        }
    } else {
        CONFIG_ERROR("unknown key \"%s\"", k);
    }
    return 0;
}

int read_dmrids(const char *filename)
{
    dmr_log_debug("noisebridge: parsing dmrids CSV %s", filename);
    FILE *fp;
    int ret;
    size_t len;
    char *line = NULL, *k = NULL, *v = NULL;
    dmr_id id;

    if ((fp = fopen(filename, "r")) == NULL) {
        dmr_log_critical("noisebridge: failed to open %s: %s", filename, strerror(errno));
        return DMR_EINVAL;
    }

    /* Parse <dmrid>,<call>[,<name>[,rest]] */
    while ((ret = getline(&line, &len, fp)) != -1) {
        if (!split(line, ",", &k, &v))
            continue;
        if ((id = atoi(k)) == 0)
            continue;
        if (!split(v, ",", &k, &v))
            continue;

        char buf[64]; /* enough space to hold call + name */
        byte_zero(buf, sizeof buf);

        char *call = k, *name = NULL;
        if (split(v, ",", &k, &v) && strlen(k) > 0) {
            name = k;
            snprintf(buf, sizeof(buf) - 1, "%s (%s)", call, name);
        } else {
            snprintf(buf, sizeof(buf) - 1, "%s", call);
        }
#if defined(DMR_TRACE)
        dmr_log_trace("noisebridge: adding id=%u, call=%s", id, buf);
#endif
        if ((ret = dmr_id_add(id, buf)) != 0)
            break;
    }
    dmr_log_info("noisebridge: loaded %lu calls", dmr_id_size());
    if (ret == -1 && errno != ENOENT) { /* ENOENT when EOF in getline */
        dmr_log_error("config: getline(%s): %s", filename, strerror(errno));
        dmr_error(DMR_EINVAL);
    }

    fclose(fp);
    return 0;
}

int read_config(char *line, char *filename, size_t lineno)
{
    dmr_log_debug("noisebridge: %s[%zu]: %s", filename, lineno, line);
    if (strlen(line) == 0 || line[0] == '#' || line[0] == ';') {
        return 0;
    }

    /* Defaults */
    config->repeater.timeout = 180;

    char *k, *v;
    if (line[strlen(line) - 1] == '{') {
        k = strtok_r(line, "{", &v);
        trim(k);
        if (k == NULL || strlen(k) == 0) {
            CONFIG_ERROR("expected section name");
        } else if (!strcmp(k, "repeater")) {
            dmr_log_debug("noisebridge: %s[%zu]: switch to section repeater", filename, lineno);
            config->section = read_config_repeater;
            return 0;
        } else if (!strcmp(k, "dmrid")) {
            dmr_log_debug("noisebridge: %s[%zu]: switch to section dmrid", filename, lineno);
            config->section = read_config_dmrid;
            return 0;
        } else if (!strcmp(k, "httpd")) {
            dmr_log_debug("noisebridge: %s[%zu]: switch to section httpd", filename, lineno);
            if (config->httpd.enabled) {
                CONFIG_ERROR("httpd already enabled");
            }
            config->section = read_config_httpd;
            config->httpd.enabled = true;
            return 0;
        } else if (!strcmp(k, "homebrew")) {
            dmr_log_debug("noisebridge: %s[%zu]: switch to section homebrew", filename, lineno);
            config->section = read_config_homebrew;
            config->proto[config->protos] = talloc_zero(config, proto_t);
            config->proto[config->protos]->type = DMR_PROTOCOL_HOMEBREW;
            config->proto[config->protos]->settings.homebrew.peer_port = 62030;
            config->proto[config->protos]->settings.homebrew.bind_port = 0; /* random */
            byte_zero(config->proto[config->protos]->settings.homebrew.peer_ip, 16);
            byte_copy(config->proto[config->protos]->settings.homebrew.bind_ip, ip6any, 16);
            config->protos++;
            return 0;
        } else if (!strcmp(k, "mbe")) {
            dmr_log_debug("noisebridge: %s[%zu]: switch to section mbe", filename, lineno);
#if !defined(WITH_MBELIB)
            CONFIG_ERROR("mbelib support not enabled");
#endif
#if !defined(HAVE_LIBPORTAUDIO)
            CONFIG_ERROR("portaudio support not enabled");
#endif
            config->section = read_config_mbe;
            config->proto[config->protos] = talloc_zero(config, proto_t);
            config->proto[config->protos]->type = DMR_PROTOCOL_MBE;
            config->proto[config->protos]->settings.mbe.quality = 3;
            config->protos++;
            return 0;
        } else if (!strcmp(k, "mmdvm")) {
            dmr_log_debug("noisebridge: %s[%zu]: switch to section mmdvm", filename, lineno);
            config->section = read_config_mmdvm;
            config->proto[config->protos] = talloc_zero(config, proto_t);
            config->proto[config->protos]->type = DMR_PROTOCOL_MMDVM;
            config->proto[config->protos]->settings.mmdvm.baud = DMR_MMDVM_BAUD;
            config->proto[config->protos]->settings.mmdvm.model = DMR_MMDVM_MODEL_G4KLX;
            config->proto[config->protos]->settings.mmdvm.color_code = 1;
            config->protos++;
            return 0;
        }
        CONFIG_ERROR("unknown section \"%s\"", k);
    } else {
        if (!split(line, "=", &k, &v)) {
            CONFIG_ERROR("syntax error \"%s\"", line);
        }
        if (k == NULL || v == NULL || strlen(k) == 0 || strlen(v) == 0) {
            CONFIG_ERROR("expected key-value pair");
        }
        dmr_log_debug("noisebridge: config %s = \"%s\"", k, v);
        if (!strcmp(k, "dmrids")) {
            if (read_dmrids(v) != 0) {
                CONFIG_ERROR("failed to parse \"%s\": %s", v, dmr_error_get());
            }
            return 0;
        } else {
            CONFIG_ERROR("unknown key \"%s\"", k);
        }
    }

    return 0;
}

int init_config(char *filename)
{
    FILE *fp = NULL;
    int ret = 0;
    char *line = NULL;
    size_t len = 0;
    size_t lineno = 0;
    ssize_t read;

    if (filename == NULL) {
        return dmr_error(DMR_EINVAL);
    }

    dmr_log_debug("noisebridge: reading %s", filename);
    if (config == NULL) {
        if ((config = talloc_zero(NULL, config_t)) == NULL ||
            (config->httpd.root = talloc_size(config, PATH_MAX)) == NULL) {
            dmr_log_critical("config: out of memory");
            ret = dmr_error(DMR_ENOMEM);
            goto bail;
        }
        config->httpd.enabled = false;
        byte_zero(config->httpd.bind, 16);
        byte_copy(config->httpd.bind, (void *)ip6mappedv4prefix, 12);
        byte_copy(config->httpd.bind + 12, (void *)ip4loopback, 4);
        config->httpd.port = 8042;
        strncpy(config->httpd.root, "html", PATH_MAX);
        config->protos = 0;
        config->section = read_config;
    }

    if (config->filename != NULL) {
        talloc_free(config->filename);
    }
    config->filename = talloc_strdup(config, filename);

    if ((fp = fopen(config->filename, "r")) == NULL) {
        dmr_log_critical("failed to open %s: %s", config->filename, strerror(errno));
        ret = -1;
        goto bail;
    }

    while ((read = getline(&line, &len, fp)) != -1) {
        lineno++;
        trim(line);
        if ((ret = config->section(line, filename, lineno)) != 0) {
            dmr_log_critical("noisebridge: %s: parse failed", filename);
            goto bail;
        }
    }

    if (config->repeater.script == NULL) {
        dmr_log_critical("noisebridge: %s: missing repeater->script", filename);
        ret = -1;
        goto bail;
    }

    if (config->protos == 0) {
        dmr_log_critical("noisebridge: %s: missing protos", filename);
        ret = -1;
        goto bail;
    }
    dmr_log_info("noisebridge: configuring %d protos", config->protos);

    goto done;

bail:
    if (config != NULL) {
        talloc_free(config);
    }

done:
    if (fp != NULL) {
        fclose(fp);
    }

    return ret;
}

config_t *load_config(void)
{
    return config;
}
