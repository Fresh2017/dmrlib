#include <ctype.h>
#include <stdlib.h>
#include <talloc.h>
#include <string.h>
#include <dmr/config.h>
#include <dmr/error.h>
#include "config.h"
#include "common/byte.h"
#include "common/format.h"
#include "common/scan.h"

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

#define CONFIG_ERROR(x,...) \
    dmr_log_critical("noisebridge: %s[%zu]: " x, filename, lineno, ##__VA_ARGS__); \
    return -1

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

int read_config_httpd(char *line, char *filename, size_t lineno)
{
    dmr_log_trace("noisebridge: %s[%zu]: (httpd) %s", filename, lineno, line);
    if (strlen(line) == 0 || line[0] == '#' || line[0] == ';') {
        return 0;
    }
    if (!strcmp(line, "}")) {
        dmr_log_trace("noisebridge: %s[%zu]: end of section httpd", filename, lineno);
        if (config->httpd.port == 0) {
            config->httpd.port = 8042;
        }
        config->section = read_config;
        return 0;
    }

    char *k = NULL, *v = NULL;
    if (!split(line, "=", &k, &v)) {
        CONFIG_ERROR("syntax error \"%s\"", line);
        return -1;
    }
    dmr_log_trace("noisebridge: config %s = \"%s\"", k, v);

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
    dmr_log_trace("noisebridge: %s[%zu]: (homebrew) %s", filename, lineno, line);
    if (strlen(line) == 0 || line[0] == '#' || line[0] == ';') {
        return 0;
    }
    if (!strcmp(line, "}")) {
        dmr_log_trace("noisebridge: %s[%zu]: end of section homebrew", filename, lineno);
        if (proto->instance.homebrew.peer_ip == NULL) {
            CONFIG_ERROR("no peer configured");
        }
        config->section = read_config;
        return 0;
    }

    char *k = NULL, *v = NULL;
    if (!split(line, "=", &k, &v)) {
        CONFIG_ERROR("syntax error \"%s\"", line);
        return -1;
    }
    dmr_log_trace("noisebridge: config %s = \"%s\"", k, v);

    CONFIG_STR(proto, "name", proto->name)
    else CONFIG_STR(proto, "auth", proto->instance.homebrew.auth)
    else CONFIG_STR(proto, "call", proto->instance.homebrew.call)
    else CONFIG_INT(proto, "repeater_id", proto->instance.homebrew.repeater_id)
    else CONFIG_INT(proto, "color_code", proto->instance.homebrew.color_code)
    else CONFIG_INT(proto, "rx_freq", proto->instance.homebrew.rx_freq)
    else CONFIG_INT(proto, "tx_freq", proto->instance.homebrew.tx_freq)
    else CONFIG_INT(proto, "tx_power", proto->instance.homebrew.tx_power)
    else CONFIG_INT(proto, "height", proto->instance.homebrew.height)
    else CONFIG_FLOAT(proto, "latitude", proto->instance.homebrew.latitude)
    else CONFIG_FLOAT(proto, "longitude", proto->instance.homebrew.longitude)
    else if (!strcmp(k, "host")) {
        char *host = v, *port = NULL;
        host = strtok_r(v, ":", &port);
        if (port == NULL) {
            proto->instance.homebrew.peer_port = 62030;
        } else {
            proto->instance.homebrew.peer_port = atoi(port);
        }
        if (proto->instance.homebrew.peer_port == 0) {
            proto->instance.homebrew.peer_port = 62030;
        }
        if (proto->instance.homebrew.peer_ip == NULL) {
            if ((proto->instance.homebrew.peer_ip = talloc_size(config, 16)) == NULL) {
                return DMR_OOM();
            }
        }
        struct hostent *hostent = NULL;
        if ((hostent = gethostbyname(host)) == NULL || hostent->h_length == 0) {
            CONFIG_ERROR("could not resolve %s: %s", host, strerror(errno));
        }
        byte_zero(proto->instance.homebrew.peer_ip, sizeof(ip6_t));
        byte_copy(proto->instance.homebrew.peer_ip, (void *)ip6mappedv4prefix, sizeof(ip6mappedv4prefix));
        byte_copy(proto->instance.homebrew.peer_ip + sizeof(ip6mappedv4prefix), (void *)hostent->h_addr_list[0], 4);
        char host_ip[FORMAT_IP6_LEN];
        byte_zero(host_ip, sizeof(host_ip));
        format_ip6(host_ip, proto->instance.homebrew.peer_ip);
        dmr_log_trace("noisebridge: %s resolved to %s", host, host_ip);
    } else {
        CONFIG_ERROR("unknown key \"%s\"", k);
    }
    return 0;
}

int read_config_mbe(char *line, char *filename, size_t lineno)
{
    proto_t *proto = config->proto[config->protos - 1];
    dmr_log_trace("noisebridge: %s[%zu]: (mbe) %s", filename, lineno, line);
    if (strlen(line) == 0 || line[0] == '#' || line[0] == ';') {
        return 0;
    }
    if (!strcmp(line, "}")) {
        dmr_log_trace("noisebridge: %s[%zu]: end of section mbe", filename, lineno);
        config->section = read_config;
        return 0;
    }

    char *k = NULL, *v = NULL;
    if (!split(line, "=", &k, &v)) {
        CONFIG_ERROR("syntax error \"%s\"", line);
        return -1;
    }
    dmr_log_trace("noisebridge: config %s = \"%s\"", k, v);
    CONFIG_STR(proto, "name", proto->name)
    else CONFIG_INT(proto, "quality", proto->instance.mbe.quality)
    else {
        CONFIG_ERROR("unknown key \"%s\"", k);
    }
    return 0;
}

int read_config_mmdvm(char *line, char *filename, size_t lineno)
{
    proto_t *proto = config->proto[config->protos - 1];
    dmr_log_trace("noisebridge: %s[%zu]: (mmdvm) %s", filename, lineno, line);
    if (strlen(line) == 0 || line[0] == '#' || line[0] == ';') {
        return 0;
    }
    if (!strcmp(line, "}")) {
        dmr_log_trace("noisebridge: %s[%zu]: end of section mmdvm", filename, lineno);
        config->section = read_config;
        return 0;
    }

    char *k = NULL, *v = NULL;
    if (!split(line, "=", &k, &v)) {
        CONFIG_ERROR("syntax error \"%s\"", line);
        return -1;
    }
    dmr_log_trace("noisebridge: config %s = \"%s\"", k, v);
    CONFIG_STR(proto, "name", proto->name)
    else CONFIG_STR(proto, "port", proto->instance.mmdvm.port)
    else if (!strcmp(k, "model")) {
        if (!strcmp(v, "generic")) {
            proto->instance.mmdvm.model = DMR_MMDVM_MODEL_GENERIC;
        } else if (!strcmp(v, "g4klx")) {
            proto->instance.mmdvm.model = DMR_MMDVM_MODEL_G4KLX;
        } else if (!strcmp(v, "dvmega")) {
            proto->instance.mmdvm.model = DMR_MMDVM_MODEL_DVMEGA;
        } else {
            CONFIG_ERROR("unknown MMDVM model \"%s\"", v);
        }
    }
    else {
        CONFIG_ERROR("unknown key \"%s\"", k);
    }
    return 0;
}

int read_config(char *line, char *filename, size_t lineno)
{
    dmr_log_trace("noisebridge: %s[%zu]: %s", filename, lineno, line);
    if (strlen(line) == 0 || line[0] == '#' || line[0] == ';') {
        return 0;
    }

    char *k, *v;
    if (line[strlen(line) - 1] == '{') {
        k = strtok_r(line, "{", &v);
        trim(k);
        if (!strcmp(k, "httpd")) {
            dmr_log_trace("noisebridge: %s[%zu]: switch to section httpd", filename, lineno);
            if (config->httpd.enabled) {
                CONFIG_ERROR("httpd already enabled");
            }
            config->section = read_config_httpd;
            config->httpd.enabled = true;
            return 0;
        } else if (!strcmp(k, "homebrew")) {
            dmr_log_trace("noisebridge: %s[%zu]: switch to section homebrew", filename, lineno);
            config->section = read_config_homebrew;
            config->proto[config->protos] = talloc_zero(config, proto_t);
            config->proto[config->protos]->type = DMR_PROTO_HOMEBREW;
            config->proto[config->protos]->instance.homebrew.peer_ip = NULL;
            config->proto[config->protos]->instance.homebrew.peer_port = 62030;
            config->proto[config->protos]->instance.homebrew.bind_ip = talloc_size(config, 16);
            config->proto[config->protos]->instance.homebrew.bind_port = 0; /* random */
            byte_copy(config->proto[config->protos]->instance.homebrew.bind_ip, (void *)ip6any, 16);
            config->protos++;
            return 0;
        } else if (!strcmp(k, "mbe")) {
            dmr_log_trace("noisebridge: %s[%zu]: switch to section mbe", filename, lineno);
#if !defined(WITH_MBELIB)
            CONFIG_ERROR("mbelib support not enabled");
#endif
#if !defined(HAVE_LIBPORTAUDIO)
            CONFIG_ERROR("portaudio support not enabled");
#endif
            config->section = read_config_mbe;
            config->proto[config->protos] = talloc_zero(config, proto_t);
            config->proto[config->protos]->type = DMR_PROTO_MBE;
            config->protos++;
            return 0;
        } else if (!strcmp(k, "mmdvm")) {
            dmr_log_trace("noisebridge: %s[%zu]: switch to section mmdvm", filename, lineno);
            config->section = read_config_mmdvm;
            config->proto[config->protos] = talloc_zero(config, proto_t);
            config->proto[config->protos]->type = DMR_PROTO_MMDVM;
            config->protos++;
            return 0;
        }
        CONFIG_ERROR("unknown section \"%s\"", k);
        return -1;
    } else {
        if (!split(line, "=", &k, &v)) {
            CONFIG_ERROR("syntax error \"%s\"", line);
            return -1;
        }
        dmr_log_trace("noisebridge: config %s = \"%s\"", k, v);
        if (!strcmp(k, "script")) {
            if (config->script != NULL) {
                dmr_log_critical("noisebridge: %s[%zu]: duplicate script", filename, lineno);
                return -1;
            }
            config->script = talloc_strdup(config, v);
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
            goto bail;
        }
    }

    if (config->script == NULL) {
        dmr_log_critical("noisebridge: %s: missing script", filename);
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
