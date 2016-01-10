#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <SDL2/SDL.h>

#include <dmr/log.h>
#include <dmr/proto.h>
#include <dmr/proto/homebrew.h>
#include <dmr/proto/mbe.h>
#include <dmr/proto/mmdvm.h>
#include <dmr/proto/repeater.h>
#include <dmr/payload/voice.h>

const char *software_id = "NoiseBridge";
#ifdef PACKAGE_ID
const char *package_id = PACKAGE_ID;
#else
const char *package_id = "NoiseBridge-git";
#endif

typedef enum {
    PEER_NONE,
    PEER_HOMEBREW,
    PEER_MBE,
    PEER_MMDVM
} peer_t;

typedef struct config_s {
    const char         *filename;
    peer_t             upstream, modem;
    dmr_homebrew_t     *homebrew;
    char               *homebrew_host_s;
    struct hostent     *homebrew_host;
    int                homebrew_port;
    char               *homebrew_auth;
    char               *homebrew_call;
    struct in_addr     homebrew_bind;
    dmr_id_t           homebrew_id;
    dmr_color_code_t   homebrew_cc;
    dmr_mmdvm_t        *mmdvm;
    char               *mmdvm_port;
    int                mmdvm_rate;
    dmr_mbe_t          *mbe;
    uint8_t            mbe_quality;
    char               *audio_device;
    dmr_log_priority_t log_level;
} config_t;

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

bool init_audio(config_t *config)
{
    int i, count;
    const char* driver_name;

    if (SDL_Init(SDL_INIT_AUDIO) != 0) {
        dmr_log_critical("unable to setup audio: %s", SDL_GetError());
        return false;
    }
    atexit(SDL_Quit);

    if (config->audio_device == NULL) {
        dmr_log("no audio_device configured, auto-detecting");
        count = SDL_GetNumAudioDevices(0);
        for (i = 0; i < count; ++i) {
            dmr_log_info("audio_device: %s", SDL_GetAudioDeviceName(i, 0));
        }
    }

    driver_name = SDL_GetCurrentAudioDriver();
    if (driver_name == NULL) {
        dmr_log_critical("audio driver not available");
        return false;
    }
    dmr_log_debug("using audio driver %s", driver_name);
    return true;
}

int stream_pos = 0;
float samples[DMR_DECODED_AMBE_FRAME_SAMPLES] = { 0 };

void stream_audio_cb(void *mem, uint8_t *stream, int len)
{
    DMR_UNUSED(mem);
    int out = 0, cur;
    while (out < len) {
        cur = min(DMR_DECODED_AMBE_FRAME_SAMPLES - stream_pos, len);
        memcpy(stream, &samples[stream_pos], cur);
        stream_pos = (stream_pos + cur) % DMR_DECODED_AMBE_FRAME_SAMPLES;
        out += cur;
    }
}

SDL_AudioDeviceID boot_audio(const char *device)
{
    SDL_AudioSpec want, have;
    SDL_AudioDeviceID dev;

    SDL_zero(want);
    want.freq = 8000;
    want.format = AUDIO_F32;
    want.channels = 1;
    want.samples = DMR_DECODED_AMBE_FRAME_SAMPLES;
    want.callback = &stream_audio_cb;

    dev = SDL_OpenAudioDevice(device, 0, &want, &have, 0);
    return dev;
}

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
            dmr_log_critical("noisebridge: %s: mmdvm_port required\n", config->filename);
            break;
        }
        if (config->mmdvm_rate == 0) {
            config->mmdvm_rate = DMR_MMDVM_BAUD_RATE;
        }

        dmr_log_info("noisebridge: connecting to MMDVM modem on port %s with %d baud\n",
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
        dmr_log_critical("noisebridge: %s: modem_type required\n", config->filename);
        break;
    }

    switch (config->upstream) {
    case PEER_HOMEBREW:
        if (!(valid = (config->homebrew_host != NULL))) {
            dmr_log_critical("noisebridge: %s: homebrew_host required\n", config->filename);
            break;
        }
        if (config->homebrew_port == 0) {
            config->homebrew_port = DMR_HOMEBREW_PORT;
        }
        if (!(valid = (config->homebrew_auth != NULL))) {
            dmr_log_critical("noisebridge: %s: homebrew_auth required\n", config->filename);
            break;
        }
        if (!(valid = (config->homebrew_call != NULL))) {
            dmr_log_critical("noisebridge: %s: homebrew_call required\n", config->filename);
            break;
        }
        if (!(valid = (config->homebrew_id != 0))) {
            dmr_log_critical("noisebridge: %s: homebrew_id required\n", config->filename);
            break;
        }
        if (!(valid = (config->homebrew_cc >= 1 || config->homebrew_cc <= 15))) {
            dmr_log_critical("noisebridge: %s: homebrew_cc required (1 >= cc >= 15)\n", config->filename);
            break;
        }


        struct in_addr **addr_list;
        struct in_addr server_addr;
        int i;
        addr_list = (struct in_addr **)config->homebrew_host->h_addr_list;
        for (i = 0; addr_list[i] != NULL; i++) {
            server_addr.s_addr = (*addr_list[0]).s_addr;
            dmr_log_debug("noisebridge: %s resolved to %s\n",
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
        dmr_log_critical("noisebridge: %s: homebrew_type required\n", config->filename);
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
    SDL_AudioDeviceID dev = 0;
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

    if ((dev = boot_audio(config->audio_device)) == 0) {
        dmr_log_critical("failed to boot audio: %s", SDL_GetError());
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
