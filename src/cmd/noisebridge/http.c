#include <dmr/malloc.h>
#include <dmr/raw.h>
#include "common/config.h"
#include "common/format.h"
#include "common/platform.h"
#include "common/socket.h"
#include "common/serial.h"
#include "config.h"
#include "http.h"
#include "http_parser.h"
#include "repeater.h"
#if defined(HAVE_FCNTL_H)
#include <fcntl.h>
#endif
#include <sys/stat.h>
#if defined(HAVE_MAGIC_H)
#include <magic.h>
#endif
#if defined(HAVE_SIGNAL_H)
#include <signal.h>
#endif
#include <sys/socket.h>
#if defined(PLATFORM_DARWIN)
#include <sys/types.h>
#endif
#if defined(HAVE_NETINET_IN_H)
#include <netinet/in.h>
#endif

#define HTTPD_MAX_CLIENTS 128
#define HTTPD_MAX_REQUEST 8192
#define HTTPD_MAX_RESPONS 1024

typedef enum {
    LIVE_NONE,
    LIVE_TS
} live_t;

typedef struct {
    ip6_t       ip;
    socket_t    *s;
	size_t		parser_pos;
    dmr_raw     *raw;
    http_parser parser;
	http_parser_settings parser_settings;
    struct {
        live_t         type;
        struct timeval last_write;
        struct {
            dmr_id src_id, dst_id;
        } ts[2];
    } live;
	struct {
		const char *header_buf;
		size_t     header_len;
		char 	   *file;
		char 	   *path;
		http_url   url;
	} request;
} client_t;

typedef struct {
    dmr_io       *io;
    socket_t	 *server;
    bool         active;
} httpd_t;

static httpd_t httpd;

//static const char *http_response = "HTTP/%d.%d %d %s\r\nServer: noisebridge\r\nDate: %s\r\nContent-Type: %s\r\nContent-Length: %lld\r\n\r\n";
static const char *http_response = "HTTP/%d.%d %d %s\r\n%s\n\r";
static const char *http_error_html = "<!doctype html><html><head><title>Error %s</title></head><body><h1>Error %s</h1></body></html>";

static struct { int status; char *message; } http_status[] = {
	{ 200, "OK"                    },
    { 301, "Moved Permanently"     },
    { 302, "Found"                 },
    { 400, "Bad Request"           },
    { 404, "Not Found"             },
    { 500, "Internal Server Error" },
    { 0,   NULL                    }
};

static char *http_status_message(int status)
{
	int i;
	for (i = 0; http_status[i].status; i++) {
		if (http_status[i].status == status) {
			return http_status[i].message;
		}
	}
	return NULL;
}

#if defined(HAVE_MAGIC_H)
magic_t magic;
#endif

static char *file_last_modified(struct stat *st)
{
    if (st == NULL)
        return NULL;

    static char datebuf[32];
    struct tm tm = *gmtime(&st->st_mtime);
	byte_zero(datebuf, sizeof datebuf);
	strftime(datebuf, sizeof datebuf, "%a, %d %b %Y %H:%M:%S %Z", &tm);	
    return datebuf;
}

static struct { char *ext; char *mime_type; } builtin_mime_types[] = {
    { "css",  "text/css"                  },
    { "gif",  "image/gif"                 },
    { "html", "text/html"                 },
    { "jpeg", "image/jpeg"                },
    { "jpg",  "image/jpeg"                },
    { "js",   "application/javascript"    },
    { "md",   "text/plain; charset=UTF-8" },
    { "png",  "image/png"                 },
    { "txt",  "text/plain"                },
    { NULL,   NULL                        }
};

static char *file_mime_type(const char *filename)
{
    char *mime_type = NULL;
#if defined(HAVE_MAGIC_H)
    if (magic != NULL) {
        mime_type = magic_file(magic, filename);
    }
#endif
    if (mime_type == NULL) {
        char *ext = (char *)format_path_ext(filename);
        if (ext != NULL) {
            ext++;
            int i;
            for (i = 0; builtin_mime_types[i].ext != NULL; i++) {
#if defined(PLATFORM_WINDOWS)
                if (!stricmp(builtin_mime_types[i].ext, ext)) {
#else
                if (!strcasecmp(builtin_mime_types[i].ext, ext)) {
#endif
                    mime_type = builtin_mime_types[i].mime_type;
                    break;
                }
            }
        }
    }
    if (mime_type == NULL) {
        return "application/octet-stream";
    }
    return mime_type;
}

typedef struct {
    char *key;
    char *value;
} header_t;

typedef struct {
    header_t **header;
    size_t   len;
    size_t   alloc;
} headers_t;

static headers_t *headers_new(void *parent)
{
    headers_t *headers = dmr_palloc(parent, headers_t);
    if (headers == NULL)
        return NULL;

    //headers->header = talloc_array(headers, header_t *, 8);
    headers->header = dmr_palloc_size(headers, sizeof(header_t *) * 8);
    headers->alloc = 8;
    return headers;
}

static int headers_add(headers_t *headers, const char *key, const char *fmt, ...)
{
    if (headers == NULL) {
        dmr_log_error("headers add: NULL pointer");
        return -1;
    }

    if ((headers->len + 1) == headers->alloc) {
        header_t **tmp = dmr_realloc(
            headers,
            headers->header,
            header_t *,
            headers->alloc + 8);

        if (tmp == NULL) {
            dmr_log_error("headers add: out of memory in realloc");
            return -1;
        }
        headers->alloc = talloc_array_length(tmp);
        headers->header = tmp;
    }

    header_t *header = dmr_palloc(headers, header_t);
    if (header == NULL) {
        dmr_log_error("headers add: out of memory");
        return -1;
    }
    header->key = dmr_strdup(header, key);

    va_list ap;
    va_start(ap, fmt);
    header->value = talloc_vasprintf(header, fmt, ap);
    va_end(ap);
    if (header->value == NULL) {
        dmr_log_error("headers add: out of memory");
        dmr_free(header);
        return -1;
    }
    headers->header[headers->len] = header;
    headers->len++;
    dmr_log_debug("headers add: %s, now %llu headers", key, headers->len);
    return 0;
}

static bool headers_contain(headers_t *headers, const char *key)
{
    size_t i;
    for (i = 0; i < headers->len; i++) {
        if (!strcmp(headers->header[i]->key, key)) {
            return true;
        }
    }
    return false;
}

static char *headers_str(headers_t *headers)
{
    if (headers->len == 0) {
        return "";
    }
    static char str[HTTPD_MAX_RESPONS];
    byte_zero(str, sizeof str);

    size_t i;
    for (i = 0; i < headers->len && strlen(str) < HTTPD_MAX_RESPONS; i++) {
        snprintf(str + strlen(str), HTTPD_MAX_RESPONS - strlen(str), "%s: %s\r\n",
            headers->header[i]->key,
            headers->header[i]->value);
    }
    dmr_log_debug("headers:\n%s", str);
    return str;
}

static int respond_header(client_t *client, int status, headers_t *headers, size_t content_length)
{
    if (headers == NULL) {
        if ((headers = headers_new(NULL)) == NULL) {
            dmr_log_error("[%s]: can't respond: out of memory", format_ip6s(client->ip));
            return -1;
        }
    }

    char *message = http_status_message(status);
    char datebuf[32];
	time_t now = time(0);
    struct tm tm = *gmtime(&now);
	byte_zero(datebuf, sizeof datebuf);
	strftime(datebuf, sizeof datebuf, "%a, %d %b %Y %H:%M:%S %Z", &tm);	

    if (headers_add(headers, "Server", "Noisebridge") == -1 ||
        headers_add(headers, "Date", datebuf) == -1) {
        dmr_free(headers);
        return -1;
    }
    if (!headers_contain(headers, "Content-Length") && content_length) {
        dmr_log_debug("adding missing Content-Length header");
        if (headers_add(headers, "Content-Length", "%lld", content_length) == -1) {
            dmr_free(headers);
            return -1;
        }
    }
    if (!headers_contain(headers, "Content-Type")) {
        dmr_log_debug("adding missing Content-Type header");
        if (headers_add(headers, "Content-Type", "text/html") == -1) {
            dmr_free(headers);
            return -1;
        }
    }

	char buf[HTTPD_MAX_RESPONS];
	byte_zero(buf, sizeof buf);
	snprintf(buf, sizeof buf, http_response,
		client->parser.http_major,
		client->parser.http_minor,
		status, message,
        headers_str(headers));

    dmr_free(headers);

	dmr_log_info("[%s]: %s %s HTTP/%d.%d %d %llu",
		format_ip6s(client->ip),
		http_method_str(client->parser.method),
		client->request.path == NULL ? "<invalid>" : client->request.path,
		client->parser.http_major,
		client->parser.http_minor,
		status,
		content_length);

	int ret;
	do {
		ret = socket_write(client->s, buf, strlen(buf));
	} while (ret == -1 && (errno == EINVAL || errno == EAGAIN));
#if defined(EPIPE)
	if (ret == -1 && errno != EPIPE) {
#else
	if (ret == -1) {
#endif
		dmr_log_error("[%s]: write failed: %s", format_ip6s(client->ip), strerror(errno));
		return -1;
	}
	
	return 0;
}

static int respond_content_write(client_t *client, char *buf, size_t len)
{
    if (len == 0) {
        len = strlen(buf);
    }
	int ret;
	do {
		ret = socket_write(client->s, buf, len);
	} while (ret == -1 && (errno == EINVAL || errno == EAGAIN));
#if defined(EPIPE)
	if (ret == -1 && errno != EPIPE) {
#else
	if (ret == -1) {
#endif
		dmr_log_error("[%s]: write failed: %s", format_ip6s(client->ip), strerror(errno));
        return -1;
	}
    return 0;
}

static int respond_error(client_t *client, int status)
{
    char *message = http_status_message(status);
	char html[HTTPD_MAX_RESPONS];
	byte_zero(html, sizeof html);
	snprintf(html, sizeof html, http_error_html, message, message);

    headers_t *headers = headers_new(NULL);
    if (headers_add(headers, "Content-Type", "text/html; charset=UTF-8") == -1) {
        dmr_free(headers);
        return -1;
    }
	if (respond_header(client, status, headers, strlen(html)) == -1)
		return -1;

    respond_content_write(client, html, 0);
    /* Always drop client */
    return -1;
}

static int respond_repeater_config(client_t *client)
{
    config_t *config = load_config();
    char json[HTTPD_MAX_RESPONS];
    off_t pos = 0;
    byte_zero(json, sizeof json);

#define S(fmt,...) do { \
    snprintf(json + pos, HTTPD_MAX_RESPONS - pos, fmt, ##__VA_ARGS__); \
    pos = strlen(json); \
} while(0)
    
    S("{\n  \"protocols\": [\n");
    size_t i;
    for (i = 0; i < config->protos; i++) {
        proto_t *proto = config->proto[i];
        if (proto == NULL)
            continue;

        if (i > 0) {
            S(",\n    {\n");
        } else {
            S("    {\n");
        }
        S("      \"name\": \"%s\",\n", proto->name);
        S("      \"active\": %s,\n", "true");
        switch (proto->type) {
        case DMR_PROTOCOL_HOMEBREW:
            S("      \"type\": \"homebrew\",\n");
            S("      \"peer\": \"%s\",\n", format_ip6s(proto->settings.homebrew.peer_ip));
            S("      \"peer_port\": %u,\n", proto->settings.homebrew.peer_port);
            S("      \"call\": \"%s\",\n", proto->settings.homebrew.call);
            S("      \"repeater_id\": %u,\n", proto->settings.homebrew.repeater_id);
            S("      \"color_code\": %u\n", proto->settings.homebrew.color_code);
            break;
        case DMR_PROTOCOL_MMDVM:
            S("      \"type\": \"mmdvm\",\n");
            S("      \"port\": \"%s\",\n", proto->settings.mmdvm.port);
            S("      \"baud\": %d,\n", proto->settings.mmdvm.baud);
            S("      \"model\": \"%s\",\n", dmr_mmdvm_model_name(proto->settings.mmdvm.model));

            serial_t *port = dmr_malloc(serial_t);
            int bus, address, vid, pid;
            if (serial_by_name(proto->settings.mmdvm.port, &port) == 0) {
                switch (serial_transport(port)) {
                case SERIAL_TRANSPORT_NATIVE:
                    S("      \"port_transport\": \"native\"\n");
                    break;

                case SERIAL_TRANSPORT_USB:
                    serial_usb_bus_address(port, &bus, &address);
                    serial_usb_vid_pid(port, &vid, &pid);
                    char *manufacturer = serial_usb_manufacturer(port);
                    char *product = serial_usb_product(port);
                    char *serial = serial_usb_serial(port);
                    S("      \"usb_bus_address\": \"%03d.%03d\",\n", bus, address);
                    S("      \"usb_id\": \"%04x:%04x\",\n", vid, pid);
                    if (manufacturer != NULL)
                        S("      \"usb_manufacturer\": \"%s\",\n", manufacturer);
                    if (product != NULL)
                        S("      \"usb_product\": \"%s\",\n", product);
                    if (serial != NULL)
                        S("      \"usb_serial\": \"%s\",\n", serial);
                    S("      \"port_transport\": \"usb\"\n");
                    break;

                case SERIAL_TRANSPORT_BLUETOOTH:
                    S("      \"port_transport\": \"bluetooth\"\n");
                    break;

                default:
                    break;
                }
            }
            if (port == NULL) {
                S("      \"port_transport\": \"unknown\"\n");
            }
            dmr_free(port);
            break;

        /*
        case DMR_PROTO_MBE:
            S("      \"type\": \"mbe\",\n");
            S("      \"device\": \"%s\",\n", proto->instance.mbe.device == NULL ? "" : proto->instance.mbe.device);
            S("      \"quality\": %d\n", proto->instance.mbe.quality);
            break;
        */

        default:
            break;
        }
        S("    }");
    }
    S("\n  ]\n}\n");
#undef S

    headers_t *headers = headers_new(NULL);
    if (headers_add(headers, "Cache-Control", "no-cache") == -1 ||
        headers_add(headers, "Content-Type", "application/json") == -1) {
        dmr_free(headers);
        return -1;
    }
    if (respond_header(client, 200, headers, strlen(json)) == -1) {
        return -1;
    }
    return respond_content_write(client, json, 0);
}

static int respond_client_live_ts_write(client_t *client)
{
    /* Lookup repeater proto */
    repeater_t *repeater = load_repeater();
    if (repeater == NULL) {
        dmr_log_error("[%s]: no repeater instance found!?", format_ip6s(client->ip));
        return -1;
    }

    dmr_ts ts;
    int written = 0;
    char data[HTTPD_MAX_RESPONS];
    size_t pos;
    time_t now = time(0);
    for (ts = 0; ts < DMR_TS_INVALID; ts++) {
        if (client->live.ts[ts].src_id == repeater->ts[ts].src_id ||
            client->live.ts[ts].dst_id == repeater->ts[ts].dst_id) {
            continue;
        }

        byte_zero(data, sizeof data);
        pos = 0;
#define S(fmt,...) do { \
    snprintf(data + pos, sizeof(data) - pos, fmt, ##__VA_ARGS__); \
    pos = strlen(data); \
} while(0)
        //S("event: ts%u\n", ts + 1);
        S("{");
        S("\"src_id\": %u, ", repeater->ts[ts].src_id);
        S("\"dst_id\": %u, ", repeater->ts[ts].dst_id);
        S("\"data_type\": %u, ", repeater->ts[ts].data_type);
        S("\"ts\": %u, ", ts);
        S("\"time\": %ld, ", now);
        if (repeater->ts[ts].state != STATE_IDLE) {
            S("\"time_recv\": %ld", repeater->ts[ts].last_frame_received.tv_sec);
        }

        S("}\n");
#undef S

        client->live.ts[ts].src_id = repeater->ts[ts].src_id;
        client->live.ts[ts].dst_id = repeater->ts[ts].dst_id;

        int ret;
        do {
            ret = socket_write(client->s, data, strlen(data));
        } while (ret == -1 && (errno == EINVAL || errno == EAGAIN));
#if defined(EPIPE)
        if (ret == EPIPE) {
            return -1;
        }
#endif
        if (ret == -1) {
            dmr_log_error("[%s]: write failed: %s", format_ip6s(client->ip), strerror(errno));
            return -1;
        }
        written++;
    }

    if (written) {
        gettimeofday(&client->live.last_write, NULL);
    }

    return 0;
}

static int handle_writable(dmr_io *io, void *clientptr, int fd)
{
    DMR_ERROR_IF_NULL(io, DMR_EINVAL);
    DMR_ERROR_IF_NULL(clientptr, DMR_EINVAL);
    DMR_UNUSED(fd);

	client_t *client = (client_t *)clientptr;

	switch (client->live.type) {
    case LIVE_NONE:
        return -1;
    case LIVE_TS:
        return respond_client_live_ts_write(client);
    }

    return -1;
}

static int respond_repeater_live_ts(client_t *client)
{
    client->live.type = LIVE_TS;

    headers_t *headers = headers_new(NULL);
    if (headers_add(headers, "Cache-Control", "no-cache")         == -1 ||
        //headers_add(headers, "Content-Type", "text/event-stream") == -1 ||
        headers_add(headers, "Content-Type", "text/octet-stream") == -1 ||
        headers_add(headers, "X-Accel-Buffering", "no")           == -1 ||
        headers_add(headers, "Access-Control-Allow-Origin", "*")  == -1) {
        dmr_free(headers);
        return -1;
    }
    if (respond_header(client, 200, headers, 0) == -1) {
        return -1;
    }

    /* Tell the io loop we want to start writing */
    //dmr_io_reg_write(io, client->s->fd, ...);
    return 0;
}

static int respond_content(client_t *client)
{
    if (client->request.path != NULL) {
        if (!strcmp(client->request.path, "/repeater/config.js")) {
            return respond_repeater_config(client);
        } else if (!strcmp(client->request.path, "/repeater/ts.stream")) {
            return respond_repeater_live_ts(client);
        }
    }
    return respond_error(client, 404);
}

static int respond_write(client_t *client)
{
    int fd;
    struct stat st;

    if (stat(client->request.file, &st) == -1) {
        dmr_log_error("[%s]: error stat %s: %s", format_ip6s(client->ip), client->request.file, strerror(errno));
        return respond_error(client, 500);
    }
    if ((fd = open(client->request.file, O_RDONLY | O_NONBLOCK)) == -1) {
        dmr_log_error("[%s]: error opening %s: %s", format_ip6s(client->ip), client->request.file, strerror(errno));
        return respond_error(client, 500);
    }

    headers_t *headers = headers_new(NULL);
    headers_add(headers, "Content-Type", file_mime_type(client->request.file));
    headers_add(headers, "Last-Modified", file_last_modified(&st));
	if (respond_header(client, 200, headers, st.st_size) == -1) {
        close(fd);
		return -1;
    }
    if (socket_sendfile_full(client->s, fd, st.st_size, NULL) == -1) {
        dmr_log_error("[%s]: error sending %s: %s", format_ip6s(client->ip), client->request.file, strerror(errno));
        close(fd);
        return -1;
    }
    close(fd);
	return -1;
}

static int handle_client_headers_complete(http_parser *parser)
{
	if (parser == NULL || parser->data == NULL)
		return -1;

	config_t *config = load_config();
	client_t *client = parser->data;
	if (client->request.url.field_data[UF_PATH].len >= PATH_MAX) {
		dmr_log_error("%s: request path out of bounds", format_ip6s(client->ip));
		return -1;
	}

	char request_path[1024];
	char request_file[PATH_MAX + 1];
	byte_zero(request_path, sizeof request_path);
	byte_zero(request_file, sizeof request_file);
	byte_copy(request_path,
		(void *)client->request.header_buf + client->request.url.field_data[UF_PATH].off,
		client->request.url.field_data[UF_PATH].len);

	if (!strncmp(request_path, "/", 2)) {
		sprintf(request_path, "/index.html");
	}
	client->request.file = dmr_palloc_size(client, PATH_MAX + 1);
	client->request.path = dmr_strdup(client, request_path);

	format_path_join(request_file, PATH_MAX, config->httpd.root, request_path);
	format_path_canonical(client->request.file, PATH_MAX, request_file);
    dmr_log_trace("[%s]: resolved %s to %s", format_ip6s(client->ip), request_file, client->request.file);

	if (strlen(client->request.file) == 0) {
        dmr_free(client->request.file);
	} else if (strncmp(config->httpd.root, client->request.file, strlen(config->httpd.root))) {
		dmr_log_warn("[%s]: attempted to request a file outside the root", format_ip6s(client->ip));
		dmr_free(client->request.file);
	}

	return 0;
}

static int handle_client_url(http_parser *parser, const char *buf, size_t len)
{
	if (parser == NULL || parser->data == NULL)
		return -1;

	client_t *client = parser->data;
	client->request.header_buf = buf;
	client->request.header_len = len;	

	return http_parser_parse_url(buf, len, 0, &client->request.url);
}

static int handle_readable(dmr_io *io, void *clientptr, int fd);

static int handle_error(dmr_io *io, void *clientptr, int fd)
{
    DMR_ERROR_IF_NULL(io, DMR_EINVAL);
    DMR_ERROR_IF_NULL(clientptr, DMR_EINVAL);

	client_t *client = (client_t *)clientptr;

	socket_close(client->s);
    dmr_io_del_read (io, fd, handle_readable);
    dmr_io_del_error(io, fd, handle_error);
    dmr_free(client);

    return 0;
}

static int handle_readable(dmr_io *io, void *clientptr, int fd)
{
    DMR_ERROR_IF_NULL(io, DMR_EINVAL);
    DMR_ERROR_IF_NULL(clientptr, DMR_EINVAL);

	client_t *client = (client_t *)clientptr;

    ssize_t ret = 0;
    do {
        ret = socket_read(client->s,
            client->raw->buf + client->raw->len,
            HTTPD_MAX_REQUEST - client->raw->len);
        if (ret != -1) {
            client->raw->len += ret;
        }
        dmr_log_trace("[%s]: read %llu/%llu", format_ip6s(client->ip), client->raw->len, HTTPD_MAX_REQUEST);
    } while (ret != 0 && (ret == -1 && (errno == EINVAL || errno == EAGAIN)));
    if (ret == -1) {
        dmr_log_error("[%s]: read failed: %s", format_ip6s(client->ip), strerror(errno));
        return -1;
    }

    size_t pos = (ssize_t)http_parser_execute(
        &client->parser,
        &client->parser_settings,
        (const char *)client->raw->buf, client->raw->len);
    if (client->parser.http_errno != 0) {
        dmr_log_error("[%s]: parser error: %s", format_ip6s(client->ip),
            http_errno_description(client->parser.http_errno));
        respond_error(client, 400);
    } else if (client->parser.upgrade) {
        dmr_log_error("[%s]: requested unsupported upgrade", format_ip6s(client->ip));
        respond_error(client, 400);
    } else if (client->raw->len != pos) {
        dmr_log_error("[%s]: unable to parse full request, dropping");
    } else if (client->request.file == NULL) {
        /* File was not found */
        if (respond_content(client) == 0) {
            /* Not done sending */
            return 0;
        }
    } else {
        /* Write requested file to the client */
        dmr_log_debug("[%s]: serving %s", format_ip6s(client->ip), client->request.file);
        respond_write(client);
    }

	/* We're done here */
	return handle_error(io, client, fd);
}

static int handle_server_accept(dmr_io *io, void *unused, int sfd)
{
    DMR_UNUSED(unused);

    dmr_log_debug("httpd: accept new client");

#if defined(HAVE_STRUCT_SOCKADDR_STORAGE)
    struct sockaddr_storage addr;
#elif defined(HAVE_STRUCT_SOCKADDR_IN6)
    struct sockaddr_in6 addr;
#else
    struct sockaddr_in addr;
#endif
    socklen_t addrlen = sizeof(addr);
	client_t *client;
    int fd;
	byte_zero(&addr, addrlen);

	if ((fd = accept(sfd, (struct sockaddr *)&addr, &addrlen)) == -1) {
		if (errno == EINTR || errno == EAGAIN) {
			return 0;
		}
		return -1;
	}

	if ((client = dmr_malloc(client_t)) == NULL) {
        close(fd);
		dmr_log_error("out of memory");
		errno = ENOMEM;
		return -1;
	}
    if ((client->raw = dmr_raw_new(HTTPD_MAX_REQUEST)) == NULL) {
        close(fd);
        dmr_log_error("out of memory");
        errno = ENOMEM;
        dmr_free(client);
        return -1;
    }
    client->request.file = NULL;
    client->request.path = NULL;

	client->s = socket_new(6, 0);
	client->s->fd = fd;
#if defined(HAVE_STRUCT_SOCKADDR_STORAGE)
    if (addr.ss_family == AF_INET || addr.ss_family == PF_INET) {
		struct sockaddr_in *sa4 = (struct sockaddr_in *)&addr;
		byte_copy(client->ip, (void *)ip6mappedv4prefix, 12);
		byte_copy(client->ip + 12, (void *)&sa4->sin_addr, 4);
    } else {
		struct sockaddr_in6 *sa6 = (struct sockaddr_in6 *)&addr;
        byte_copy(client->ip, (void *)&sa6->sin6_addr, 16);
    }
#elif defined(HAVE_STRUCT_SOCKADDR_IN6)
    if (addr.sin6_family == AF_INET || addr.sin6_family == PF_INET) {
		struct sockaddr_in *sa4 = (struct sockaddr_in *)&addr;
		byte_copy(client->ip, (void *)ip6mappedv4prefix, 12);
		byte_copy(client->ip + 12, (void *)&sa4->sin_addr, 4);
    } else {
        byte_copy(client->ip, (void *)&addr.sin6_addr, 16);
    }
#else
	byte_copy(client->ip, (void *)ip6mappedv4prefix, 12);
	byte_copy(client->ip + 12, (void *)&addr.sin_addr, 4);
#endif
	dmr_log_debug("[%s]: new client on fd %d", format_ip6s(client->ip), fd);
	if (socket_set_blocking(client->s, 0) == -1) {
        close(fd);
        dmr_free(client);
		dmr_log_errno("socket set blocking off");
		return -1;
	}
	if (socket_set_nopipe(client->s, 1) == -1) {
        close(fd);
        dmr_free(client);
		dmr_log_errno("socket set nopipe on");
		return -1;
	}

	byte_zero(&client->parser, sizeof(struct http_parser));
	byte_zero(&client->parser_settings, sizeof(struct http_parser_settings));
	http_parser_init(&client->parser, HTTP_REQUEST);
    http_url_init(&client->request.url);
	client->parser.data = client;
	client->parser_settings.on_headers_complete = handle_client_headers_complete;
    client->parser_settings.on_url = handle_client_url;

    dmr_io_reg_read (io, fd, handle_readable, client, true);
    dmr_io_reg_error(io, fd, handle_error,    client, true);

	return 0;
}

static int handle_server_error(dmr_io *io, void *unused, int sfd)
{
    DMR_UNUSED(io);
    DMR_UNUSED(unused);
    DMR_UNUSED(sfd);
    dmr_log_critical("httpd: error on server socket: %s", strerror(errno));
    return 1;
}

void stop_http(void)
{
    httpd.active = false;
}

int start_http(void *unused)
{
	(void)unused;

    config_t *config = load_config();
    int ret = 0;

	dmr_log_info("httpd: starting on http://[%s]:%u",
		format_ip6s(config->httpd.bind),
		config->httpd.port);

	if ((ret = socket_bind(httpd.server, config->httpd.bind, config->httpd.port)) == -1)
		goto bail;
	if ((ret = socket_listen(httpd.server, HTTPD_MAX_CLIENTS)) == -1)
		goto bail;

    /* io callback for new clients */
    if ((ret = dmr_io_reg_read (httpd.io, httpd.server->fd, handle_server_accept, NULL, false)) != 0)
        goto bail;
    if ((ret = dmr_io_reg_error(httpd.io, httpd.server->fd, handle_server_error, NULL, false)) != 0)
        goto bail;

    return 0;

bail:
    dmr_log_critical("httpd: start failed: %s", strerror(errno));
    return -1;
}

int init_http(dmr_io *io)
{
    config_t *config = load_config();
    if (!config->httpd.enabled) {
        dmr_log_info("httpd: not enabled");
        return 0;
    }

    httpd.io = io;

#if defined(HAVE_MAGIC_H)
    if ((magic = magic_open(MAGIC_MIME_TYPE | MAGIC_MIME_ENCODING)) == NULL) {
        dmr_log_warn("magic: %s", strerror(errno));
    } else {
        if (magic_load(magic, NULL) == -1) {
            dmr_log_warn("magic load: %s", strerror(errno));
            magic_close(magic);
            magic = NULL;
        } else if (magic_compile(magic, NULL) == -1) {
            dmr_log_warn("magic compile: %s", strerror(errno));
            magic_close(magic);
            magic = NULL;
        }
    }
#endif

    if ((httpd.server = socket_tcp6(0)) == NULL) {
		dmr_log_error("httpd: out of memory");
        goto bail;
    }
	if (socket_set_blocking(httpd.server, 0) == -1) {
		dmr_log_errno("socket set reuseport");
    }
	if (socket_set_ipv6only(httpd.server, 0) == -1) {
		dmr_log_errno("socket set ipv6only");
    }
	if (socket_set_reuseaddr(httpd.server, 1) == -1) {
		dmr_log_errno("socket set reuseaddr");
    }
	if (socket_set_reuseport(httpd.server, 1) == -1) {
		dmr_log_errno("socket set reuseport");
    }

    return start_http(io);

bail:
    dmr_free(httpd.server);
    return -1;
}
