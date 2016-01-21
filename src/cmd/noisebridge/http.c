#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <netinet/in.h>
#include <dmr/error.h>
#include <dmr/malloc.h>
#include "http.h"
#include "config.h"

#define HTTP_BUFFER_SIZE 8192

static http_t server;
static char *localhost = "127.0.0.1";

struct http_client {
    int                fd;
    char               buf[HTTP_BUFFER_SIZE];
    struct sockaddr_in addr;
    size_t             out;
    http_status_t      status;
    dmr_thread_t       thread;
};

struct {
    http_status_t status;
    char          *message;
}  http_status_message[] = {
    { HTTP_OK,          "OK" },
    { HTTP_BAD_REQUEST, "BAD REQUEST" },
    { HTTP_NOT_FOUND,   "NOT FOUND" },
    { HTTP_ERROR,       "INTERNAL SERVER ERROR" },
    { 0, NULL }, /* sentinel */
};

struct {
    char *path;
    char *content_type;
} http_static[] = {
    { "index.html",     "text/html; charset=UTF-8" },
    { "noisebridge.js", "application/javascript"   },
    { NULL, NULL } /* sentinel */
};

bool init_http(void)
{
    config_t *config = load_config();
    if (config == NULL)
        return false;

    server.bind.sin_family = AF_INET;
    if (config->http_addr != NULL) {
        if (inet_aton(config->http_addr, &server.bind.sin_addr) != 1) {
            dmr_log_critical("http: failed to parse IP %s: %s", config->http_addr, strerror(errno));
            return false;
        }
    } else {
        if (inet_aton(localhost, &server.bind.sin_addr) != 1) {
            dmr_log_critical("http: failed to parse IP %s: %s", localhost, strerror(errno));
            return false;
        }
    }
    server.bind.sin_port = htons(config->http_port);

    if ((server.fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        dmr_log_critical("http: failed to create socket: %s", strerror(errno));
        return false;
    }

    int enable = 1;
    setsockopt(server.fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable));
    setsockopt(server.fd, SOL_SOCKET, SO_REUSEPORT, &enable, sizeof(enable));

    if (bind(server.fd, (struct sockaddr *)&server.bind, sizeof(struct sockaddr_in)) != 0) {
        dmr_log_critical("http: failed to bind %s:%d: %s",
            inet_ntoa(server.bind.sin_addr), config->http_port,
            strerror(errno));
        return false;
    }
    dmr_log_info("http: listening on http://%s:%d/",
            inet_ntoa(server.bind.sin_addr), config->http_port);

    return true;
}

static size_t http_respond(struct http_client *client, http_status_t status, char *content_type, char *fmt, ...)
{
    if (client == NULL || fmt == NULL)
        return 0;

    client->status = status;
    char *status_message = "BAD REQUEST";
    int i;
    for (i = 0; http_status_message[i].message != NULL; i++) {
        if (http_status_message[i].status == status) {
            status_message = http_status_message[i].message;
            break;
        }
    }

    char buf[HTTP_BUFFER_SIZE];
    memset(buf, 0, sizeof(buf));
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    size_t len = strlen(buf);
    char out[HTTP_BUFFER_SIZE];
    memset(out, 0, sizeof(out));
    snprintf(out, sizeof(out), "HTTP/1.0 %d %s\r\nContent-Type: %s\r\nContent-Length: %ld\r\n\r\n%s",
        status, status_message,
        (content_type == NULL) ? "text/html" : content_type,
        len, buf);

    return write(client->fd, out, strlen(out));
}

static int http_start_client_thread(void *clientptr)
{
    int ret = dmr_thread_success;
    char *path = NULL, *request_path = NULL;
    FILE *fp = NULL;
    size_t out = 0;

    struct http_client *client = (struct http_client *)clientptr;
    if (client == NULL) {
        dmr_log_error("http: can't start client thread: received NULL");
        ret = dmr_thread_error;
        goto stop;
    }

    client->status = HTTP_BAD_REQUEST;
    size_t len;
    if ((len = recv(client->fd, client->buf, sizeof(client->buf), 0)) == 0) {
        dmr_log_error("http: recv(): %s", strerror(errno));
        ret = dmr_thread_error;
        goto stop;
    }
    if (len > 0 && len < sizeof(client->buf)) {
        client->buf[len] = 0;
    } else {
        client->buf[0] = 0;
    }

    // Only GET supported
    if (len < 4 || strncmp(client->buf, "GET ", 4)) {
        dmr_log_error("http: %s: received non-GET request", inet_ntoa(client->addr.sin_addr));
        out = http_respond(client, HTTP_BAD_REQUEST, NULL, "Bad request, only HTTP GET is supported.");
        goto stop;
    }

    // If the requested path doesn't start with a slash, serve the main page.
    len -= 4;
    memmove(client->buf, client->buf + 4, len);
    if (client->buf[0] != '/') {
        dmr_log_trace("http: forcing to GET /");
        strcpy(client->buf, "/ HTTP/1.0");
    }

    // Just keep "GET /<path>"
    dmr_log_trace("http: looking for path end");
    size_t pos;
    for (pos = 0; client->buf[pos] != 0 && client->buf[pos] != ' ' && pos < len; pos++);
    client->buf[pos] = 0;
    request_path = client->buf;

    char *filename = NULL, *content_type = NULL;
    if (!strcmp(request_path, "/")) {
        filename = "index.html";
    } else {
        for (pos = 0; http_static[pos].path != NULL; pos++) {
            if (!strcmp(&request_path[1], http_static[pos].path)) {  // Skip leading /
                filename = http_static[pos].path;
                content_type = http_static[pos].content_type;
                break;
            }
        }
    }

    if (filename == NULL) {
        dmr_log_error("http: %s not found", path);
        out = http_respond(client, HTTP_NOT_FOUND, NULL, "Requested path not found.");
        goto stop;
    }

    config_t *config = load_config();
    path = strdup(config->http_root);
    if (path == NULL) {
        dmr_error(DMR_ENOMEM);
        goto stop;
    }
    path = join_path(path, filename);
    if (path == NULL) {
        dmr_error(DMR_ENOMEM);
        goto stop;
    }
    dmr_log_trace("http: serving %s", path);
    if ((fp = fopen(path, "r")) == NULL) {
        dmr_log_error("http: open %s: %s", path, strerror(errno));
        out = http_respond(client, HTTP_NOT_FOUND, NULL, "Requested path not found.");
        goto stop;
    }

    char buf[HTTP_BUFFER_SIZE];
    memset(buf, 0, sizeof(buf));
    if (fread(buf, sizeof(buf), 1, fp) < 18) { /* Minial HTTP/1.1 GET request size */
        dmr_log_error("http: fread(%s): %s", path, strerror(errno));
        out = http_respond(client, HTTP_ERROR, NULL, "Error reading file (see log).");
        goto stop;
    }

    out = http_respond(client, HTTP_OK, content_type, "%s", buf);

stop:
    if (out > 0 && request_path != NULL) {
        dmr_log_info("http: %s: GET %s %d %d", inet_ntoa(client->addr.sin_addr), request_path, client->status, out);
        free(path);
    }

    if (fp != NULL)
        fclose(fp);

    if (client->fd > 0)
        close(client->fd);

    dmr_thread_exit(ret);
    return ret;
}

static int http_start_thread(void *ptr)
{
    DMR_UNUSED(ptr);
    dmr_log_trace("http: start thread");
    int ret = dmr_thread_success;

    if (listen(server.fd, 10) < 0) {
        dmr_log_error("http: listen(): %s", strerror(errno));
        ret = dmr_thread_error;
        goto stop;
    }

    while (1) {
        struct http_client *client = dmr_malloc(struct http_client);
        if (client == NULL) {
            dmr_error(DMR_ENOMEM);
            ret = dmr_thread_error;
            goto stop;
        }

        socklen_t addrlen = 0;
        if ((client->fd = accept(server.fd, (struct sockaddr *)&client->addr, &addrlen)) == 0) {
            dmr_log_error("http: accept(): %s", strerror(errno));
            dmr_free(client);
            ret = dmr_thread_error;
            goto stop;
        }
        dmr_log_debug("http: accepted %s:%d", inet_ntoa(client->addr.sin_addr), ntohs(client->addr.sin_port));
        if (dmr_thread_create(&client->thread, http_start_client_thread, client) != dmr_thread_success) {
            dmr_log_critical("http: can't create client thread");
            return false;
        }
    }

stop:
    if (server.fd > 0)
        close(server.fd);
    dmr_thread_exit(ret);
    return ret;
}


bool boot_http(void)
{
    if (dmr_thread_create(&server.thread, http_start_thread, NULL) != dmr_thread_success) {
        dmr_log_critical("http: can't creat thread");
        return false;
    }
    return true;
}

void kill_http(void)
{
    if (server.fd > 0) {
        close(server.fd);
        server.fd = -1;
    }
    if (server.thread > 0) {
        int res;
        dmr_thread_join(server.thread, &res);
        dmr_log_info("http: thread join: %d", res);
    }
}
