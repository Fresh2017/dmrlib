#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <dmr/proto/homebrew.h>
#include <dmr/proto/mmdvm.h>

typedef enum {
    PEER_NONE,
    PEER_HOMEBREW,
    PEER_MMDVM
} peer_t;

typedef struct config_s {
    peer_t           upstream, modem;
    dmr_homebrew_t   *homebrew;
    char             *homebrew_host_s;
    struct hostent   *homebrew_host;
    int              homebrew_port;
    char             *homebrew_auth;
    char             *homebrew_call;
    struct in_addr   homebrew_bind;
    dmr_id_t         homebrew_id;
    dmr_color_code_t homebrew_cc;
    dmr_mmdvm_t      *mmdvm;
    char             *mmdvm_port;
    int              mmdvm_rate;
} config_t;

#define HEXDUMP_COLS 16
void dump_hex(void *mem, unsigned int len)
{
    unsigned int i, j;
    for(i = 0; i < len + ((len % HEXDUMP_COLS) ? (HEXDUMP_COLS - len % HEXDUMP_COLS) : 0); i++) {
        /* print offset */
        if (i % HEXDUMP_COLS == 0) {
            printf("0x%06x: ", i);
        }

        /* print hex data */
        if (i < len) {
            printf("%02x ", 0xFF & ((char*)mem)[i]);
        } else { /* end of block, just aligning for ASCII dump */
            printf("   ");
        }

        /* print ASCII dump */
        if (i % HEXDUMP_COLS == (HEXDUMP_COLS - 1)) {
            for (j = i - (HEXDUMP_COLS - 1); j <= i; j++) {
                if (j >= len) { /* end of block, not really printing */
                    putchar(' ');
                } else if (isprint(((char*)mem)[j])) { /* printable char */
                    putchar(0xff & ((char*)mem)[j]);
                } else { /* other char */
                    putchar('.');
                }
            }
            putchar('\n');
        }
    }
}

char *trim(char *str)
{
    char *end;

    if (str == NULL)
        return NULL;

    // Trim leading space
    while (isspace(*str)) str++;

    if (*str == 0)  // All spaces?
        return str;

    // Trim trailing space
    end = str + strlen(str) - 1;
    while (end > str && isspace(*end)) end--;

    // Write new null terminator
    *(end+1) = 0;

    return str;
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

    if ((fp = fopen(filename, "r")) == NULL) {
        fprintf(stderr, "noisebridge: failed to open %s: %s\n",
            filename, strerror(errno));
        return NULL;
    }

    while ((read = getline(&line, &len, fp)) != -1) {
        lineno++;
        line = trim(line);

        if (strlen(line) == 0 || line[0] == '#' || line[0] == ';')
            continue;

        k = strtok_r(line, "=", &v);
        k = trim(k);
        v = trim(v);
        if (k == NULL || v == NULL || strlen(v) == 0) {
            fprintf(stderr, "%s[%zu]: syntax error\n", filename, lineno);
            valid = false;
            break;
        }

        //printf("%s = %s\n", k, v);
        if (!strcmp(k, "upstream_type")) {
            if (!strcmp(v, "homebrew")) {
                config.upstream = PEER_HOMEBREW;
            } else {
                fprintf(stderr, "%s[%zu]: unsupported type %s\n", filename, lineno, v);
                valid = false;
                break;
            }

        } else if (!strcmp(k, "homebrew_host")) {
            config.homebrew_host = gethostbyname(v);
            if (config.homebrew_host == NULL) {
                fprintf(stderr, "%s[%zu]: unresolved name %s\n", filename, lineno, v);
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
            } else {
                fprintf(stderr, "%s[%zu]: unsupported type %s\n", filename, lineno, v);
                valid = false;
                break;
            }

        } else if (!strcmp(k, "mmdvm_port")) {
            config.mmdvm_port = strdup(v);

        } else if (!strcmp(k, "mmdvm_rate")) {
            config.mmdvm_rate = atoi(v);

        } else {
            fprintf(stderr, "%s[%zu]: syntax error\n", filename, lineno);
            valid = false;
            break;
        }
    }
    fclose(fp);

    switch (config.modem) {
    case PEER_MMDVM:
        if (!(valid = (config.mmdvm_port != NULL))) {
            fprintf(stderr, "%s: mmdvm_port required\n", filename);
            break;
        }
        if (config.mmdvm_rate == 0) {
            config.mmdvm_rate = DMR_MMDVM_BAUD_RATE;
        }

        printf("noisebridge: connecting to MMDVM modem on port %s with %d baud\n",
            config.mmdvm_port, config.mmdvm_rate);
        config.mmdvm = dmr_mmdvm_open(config.mmdvm_port, config.mmdvm_rate, 1000UL);
        if (!dmr_mmdvm_sync(config.mmdvm)) {
            valid = false;
            fprintf(stderr, "%s: mmdvm modem sync failed\n", filename);
            return NULL;
        }
        break;

    default:
        valid = false;
        fprintf(stderr, "%s: modem_type required\n", filename);
        break;
    }

    switch (config.upstream) {
    case PEER_HOMEBREW:
        if (!(valid = (config.homebrew_host != NULL))) {
            fprintf(stderr, "%s: homebrew_host required\n", filename);
            break;
        }
        if (config.homebrew_port == 0) {
            config.homebrew_port = DMR_HOMEBREW_PORT;
        }
        if (!(valid = (config.homebrew_auth != NULL))) {
            fprintf(stderr, "%s: homebrew_auth required\n", filename);
            break;
        }
        if (!(valid = (config.homebrew_call != NULL))) {
            fprintf(stderr, "%s: homebrew_call required\n", filename);
            break;
        }
        if (!(valid = (config.homebrew_id != 0))) {
            fprintf(stderr, "%s: homebrew_id required\n", filename);
            break;
        }
        if (!(valid = (config.homebrew_cc >= 1 || config.homebrew_cc <= 15))) {
            fprintf(stderr, "%s: homebrew_cc required (1 >= cc >= 15)\n", filename);
            break;
        }


        struct in_addr **addr_list;
        struct in_addr server_addr;
        addr_list = (struct in_addr **)config.homebrew_host->h_addr_list;
        for (int i = 0; addr_list[i] != NULL; i++) {
            server_addr.s_addr = (*addr_list[0]).s_addr;
            printf("noisebridge: %s resolved to %s\n",
                config.homebrew_host->h_name,
                inet_ntoa(server_addr));

            /*
            printf("noisebridge: connecting to HomeBrew repeater at %s:%d on %s\n",
                inet_ntoa(server_addr), config.homebrew_port,
                inet_ntoa(config.homebrew_bind));
            */
            config.homebrew = dmr_homebrew_new(
                config.homebrew_bind,
                config.homebrew_port,
                server_addr);
            dmr_homebrew_config_callsign(config.homebrew->config, config.homebrew_call);
            dmr_homebrew_config_repeater_id(config.homebrew->config, config.homebrew_id);
            dmr_homebrew_config_color_code(config.homebrew->config, config.homebrew_cc);
            if (config.mmdvm != NULL && config.mmdvm->firmware != NULL) {
                dmr_homebrew_config_software_id(config.homebrew->config, config.mmdvm->firmware);
                bool zero = false;
                for (size_t i = 0; i < sizeof(config.homebrew->config->software_id); i++) {
                    if (zero) {
                        config.homebrew->config->software_id[i] = 0x20;
                    } else if (config.homebrew->config->software_id[i] == '(') {
                        config.homebrew->config->software_id[i] = 0x20;
                        config.homebrew->config->software_id[i - 1] = 0x20;
                        zero = true;
                    } else if (config.homebrew->config->software_id[i] == ' ') {
                        config.homebrew->config->software_id[i] = '-';
                    }
                }
            }
            dump_hex(&config.homebrew->config, sizeof(dmr_homebrew_config_t));
            printf("%02x%02x%02x%02x\n",
                config.homebrew->config->repeater_id[0],
                config.homebrew->config->repeater_id[1],
                config.homebrew->config->repeater_id[2],
                config.homebrew->config->repeater_id[3]);
            if (dmr_homebrew_auth(config.homebrew, config.homebrew_auth))
                break;

            free(config.homebrew);
            config.homebrew = NULL;
        }

        if (config.homebrew != NULL) {
            dmr_homebrew_loop(config.homebrew);
        }

        break;

    default:
        valid = false;
        fprintf(stderr, "%s: homebrew_type required\n", filename);
        break;
    }

    if (!valid)
        return NULL;

    return &config;
}

int main(int argc, char **argv)
{
    config_t *config;

    if (argc != 2) {
        fprintf(stderr, "%s <config>\n", argv[0]);
        return 1;
    }

    config = configure(argv[1]);
    if (config == NULL)
        return 1;

    return 0;
}
