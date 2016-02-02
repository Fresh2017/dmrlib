#include <talloc.h>
#include <time.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <dmr/log.h>
#include <dmr/thread.h>
#include "config.h"
#include "http.h"
#include "http_parser.h"
#include "common/config.h"
#include "common/format.h"
#include "common/socket.h"
#include "common/uint.h"
#include "common/platform.h"

static httpd_t *httpd = NULL;
static const char *localhost = "localhost";
static struct {
	int  code;
	char *message;
} http_codes[] = {
	{ 200, "OK" 		 		   },
	{ 301, "Moved Permanently"     },
	{ 302, "Found"                 },
	{ 400, "Bad Request" 		   },
	{ 404, "Not Found"   		   },
	{ 500, "Internal Server Error" },
	{ 0,   NULL 				   }
};
static struct {
	int  method;
	char *name;
} http_methods[] = {
	{ HTTP_DELETE, "DELETE" },
	{ HTTP_GET,    "GET"    },
	{ HTTP_HEAD,   "HEAD"   },
	{ HTTP_POST,   "POST"   },
	{ -1,          NULL     }
};
static struct {
	char *ext;
	char *content_type;
} content_types[] = {
	{ "css",  "text/css" },
    { "gif",  "image/gif" },
	{ "html", "text/html" },
    { "jpg",  "image/jpeg" },
	{ "js",   "application/javascript" },
    { "png",  "image/png" },
	{ NULL,    NULL }
};

static const char *http_response = "HTTP/%d.%d %d %s\r\nServer: noisebridge\r\nDate: %s\r\nContent-Type: %s\r\nContent-Length: %lld\r\n\r\n";
static const char *http_error_html = "<!doctype html><html><head><title>Error %s</title></head><body><h1>Error %s</h1></body></html>";

static char *http_code_message(int code)
{
	uint8_t i;
	for (i = 0; http_codes[i].message != NULL; i++) {
		if (http_codes[i].code == code) {
			return http_codes[i].message;
		}
	}
	return "unsupported";
}

static char *http_content_type(const char *filename)
{
	const char *ext = format_path_ext(filename);
	if (ext != NULL) {
        ext++;
        dmr_log_debug("scanning content type for \"%s\"", ext);
		uint8_t i;
		for (i = 0; content_types[i].ext != NULL; i++) {
			if (!strncmp(content_types[i].ext, ext, strlen(content_types[i].ext))) {
				return content_types[i].content_type;
			}
		}
	}
	return NULL;
}

static char *http_method_name(int method)
{
	uint8_t i;
	for (i = 0; http_methods[i].name != NULL; i++) {
		if (http_methods[i].method == method) {
			return http_methods[i].name;
		}
	}
	return "unsupported";
}

client_t *new_client(int fd)
{
	if (httpd->clients < HTTP_CLIENT_MAX) {
		uint8_t i = 0;
		while (httpd->client[i] != 0) {
			i++;
		}
		client_t *client = NULL;
		if ((client = talloc_zero(NULL, client_t)) == NULL) {
			DMR_OOM();
			return NULL;
		}
        if ((client->buf = talloc_zero_size(client, HTTP_REQUEST_MAX)) == NULL) {
            DMR_OOM();
            TALLOC_FREE(client);
            return NULL;
        }
        client->bufpos = 0;
        client->buflen = HTTP_REQUEST_MAX;
        client->readfd = -1;
        if ((client->s = socket_new(6, 0)) == NULL) {
            DMR_OOM();
			TALLOC_FREE(client);
			return NULL;
		}
        client->s->fd = fd;
		if ((client->thread = talloc_zero(client, dmr_thread_t)) == NULL) {
			DMR_OOM();
			TALLOC_FREE(client);
			return NULL;
		}
		client->request_path = NULL;
		client->header = NULL;
		client->headerlen = 0;
		httpd->client[i] = client;
		httpd->clients++;
		return client;
	}

	dmr_log_error("no client slots available");
	close(fd);
	return NULL;
}

void del_client(client_t *client)
{
	uint8_t i;
	for (i = 0; i < HTTP_CLIENT_MAX; i++) {
		if (httpd->client[i] == client) {
			httpd->client[i] = NULL;
			httpd->clients--;
			break;
		}
	}

	if (client->s != NULL) {
        if (client->s->fd > 0) {
            socket_close(client->s);
        }
        TALLOC_FREE(client->s);
	}
	TALLOC_FREE(client);
}

int do_http_header(http_parser *parser, int code, const char *content_type, size_t content_length)
{
	if (parser == NULL || parser->data == NULL)
			return dmr_error(DMR_EINVAL);

	client_t *client = parser->data;
	
	char buf[HTTP_RESPONSE_MAX], tmbuf[128];
	time_t now = time(0);
	struct tm tm = *gmtime(&now);

	byte_zero(tmbuf, sizeof tmbuf);
	strftime(tmbuf, sizeof tmbuf, "%a, %d %b %Y %H:%M:%S %Z", &tm);

	byte_zero(buf, sizeof buf);
	snprintf(buf, sizeof buf, http_response,
		parser->http_major, parser->http_minor,
		code, http_code_message(code), tmbuf,
		(content_type == NULL ? "text/html" : content_type),
        content_length);

	char ip[FORMAT_IP6_LEN];
	byte_zero(ip, sizeof ip);
	format_ip6(ip, client->ip);
	dmr_log_info("%s %s %s HTTP/%d.%d %d %llu",
		ip, http_method_name(parser->method),
		(client->request_path == NULL ? "?" : client->request_path),
		parser->http_major, parser->http_minor,
        code, content_length);

	if (write(client->s->fd, (uint8_t *)buf, strlen(buf)) <= 0) {
		dmr_log_error("write(): %s", strerror(errno));
		return -1;
	}

	return 0;
}

int do_http_response(http_parser *parser, int code, const char *content_type, const char *content, size_t content_length)
{
	if (parser == NULL || parser->data == NULL)
		return dmr_error(DMR_EINVAL);

	if (do_http_header(parser, code, content_type, content_length) != 0) {
		return -1;
	}
	client_t *client = (client_t *)parser->data;
	if (socket_write(client->s, content, content_length) <= 0) {
		dmr_log_error("write(): %s", strerror(errno));
		return -1;
	}
	return 0;
}

int do_http_error(http_parser *parser, int code)
{
	char buf[1024], *message = http_code_message(code);
	byte_zero(buf, sizeof buf);
	snprintf(buf, sizeof buf, http_error_html, message, message);

    /* Always return non-zero */
	int ret = do_http_response(parser, code, "text/html", buf, strlen(buf) - 1);
    if (ret == 0) {
        return 1;
    }
    return ret;
}

int do_http_sendfile(http_parser *parser, const char *filename)
{
	int fd;
	struct stat st;
	client_t *client = parser->data;
    char *content_type = http_content_type(filename);
    if (content_type == NULL) {
        dmr_log_error("can't serve %s: unknown content type", filename);
        return do_http_error(parser, 404);
    }
	if (stat(filename, &st) != 0) {
		dmr_log_error("can't serve %s: stat(): %s", filename, strerror(errno));
		return do_http_error(parser, 404);
	}
	if ((fd = open(filename, O_RDONLY|O_NONBLOCK)) <= 0) {
		dmr_log_error("can't serve %s: fopen(): %s", filename, strerror(errno));
		return do_http_error(parser, 500);
	}
	if (do_http_header(parser, 200, content_type, st.st_size) != 0) {
		return -1;
	}
    //client->readfd = fd;

    if (socket_sendfile_full(client->s, fd, st.st_size, NULL) != 0) {
        dmr_log_error("can't serve %s: sendfile(): %s", filename, strerror(errno));
        close(fd);
        return -1;
    }

    // We're done here
    shutdown(client->s->fd, SHUT_RDWR);
	return 1;
}

int on_url(http_parser *parser, const char *buf, size_t len)
{
	if (parser == NULL || parser->data == NULL)
		return dmr_error(DMR_EINVAL);

	client_t *client = parser->data;
	client->header = buf;
	client->headerlen = len;
	return http_parser_parse_url(buf, len, 0, &client->url);
}

int on_headers_complete(http_parser *parser)
{
	if (parser == NULL || parser->data == NULL)
		return dmr_error(DMR_EINVAL);

	config_t *config = load_config();
	client_t *client = parser->data;

	char request_path[1025], path[PATH_MAX], abspath[PATH_MAX + 1];
	byte_zero(request_path, sizeof request_path);
	byte_copy(request_path, (void *)client->header + client->url.field_data[UF_PATH].off, client->url.field_data[UF_PATH].len);
	if (request_path == NULL) {
		return DMR_OOM();
	}
	if (strlen(request_path) == 0 || strlen(request_path) >= 1023) {
        dmr_log_error("client request path out of bounds");
		return 0;
	}
	if (!strncmp(request_path, "/", 1024)) {
		strncpy(request_path, "/index.html", 1024);
	}
    client->request_file = NULL;
	if ((client->request_path = talloc_strdup(client, request_path)) == NULL) {
		return DMR_OOM();
	}

	format_path_join(path, PATH_MAX, config->httpd.root, request_path);
	format_path_canonical(abspath, PATH_MAX, path);
	/* format_path_canonical uses readlink(), so it returns an empty string if
	 * the target file name does not exist */
	if (strlen(abspath) == 0) {
        dmr_log_error("client requested non-existing file");
	} else if (strncmp(config->httpd.root, abspath, strlen(config->httpd.root))) {
		dmr_log_error("client attempted to request outside httpd root");
	} else {
        client->request_file = talloc_strdup(client, abspath);
    }
	return 0;
}

int on_message_begin(http_parser *parser)
{
	dmr_log_debug("message begin %p", parser);
	return 0;
}

int on_message_complete(http_parser *parser)
{
	dmr_log_debug("message complete %p", parser);
	if (parser == NULL || parser->data == NULL)
		return dmr_error(DMR_EINVAL);
    return 0;
}

int respond_to_client(client_t *client)
{
    /* Set up callbacks */
    http_parser parser;
    byte_zero(&parser, sizeof parser);
    parser.data = client;
	http_parser_init(&parser, HTTP_REQUEST);

	http_parser_settings settings;
	byte_zero(&settings, sizeof settings);
	settings.on_url = on_url;
	settings.on_headers_complete = on_headers_complete;
	settings.on_message_begin = on_message_begin;
    settings.on_message_complete = on_message_complete;

    dmr_log_debug("client requested %d bytes:\n%.*s", client->bufpos, client->bufpos, client->buf);
	size_t pos = (ssize_t)http_parser_execute(&parser, &settings, client->buf, client->bufpos);
	if (parser.http_errno != 0) {
		dmr_log_error("parser error %s", http_errno_description(parser.http_errno));
		return do_http_error(&parser, 400);
	} else if (parser.upgrade) {
        dmr_log_error("upgrade not supported");
		return do_http_error(&parser, 400);
	} else if (pos != client->bufpos) {
		dmr_log_error("could not parse all requested bytes, dropping connection");
        return 1;
	}
    if (client->request_file != NULL) {
        return do_http_sendfile(&parser, client->request_file);
    }
    dmr_log_error("no acceptable file requested");
    return do_http_error(&parser, 404);
}

int accept_client(void)
{
#if defined(HAVE_LIBC_IPV6)
    struct sockaddr_in6 sa = { 0, };
#else
    struct sockaddr_in sa = { 0, };
#endif
    socklen_t salen = 0;
    int fd;

    client_t *client = NULL;
    if ((client = talloc_zero(httpd, client_t)) == NULL) {
        DMR_OOM();
        goto bail;
    }
    if ((fd = accept(httpd->s->fd, (struct sockaddr *)&sa, &salen)) < 0) {
        dmr_log_error("accept(): %s", strerror(errno));
        goto bail;
    }
    if ((client = new_client(fd)) == NULL) {
        dmr_log_error("could not initialize new client");
        goto bail;
    }
    uint16_t port;
#if defined(HAVE_LIBC_IPV6)
    if (sa.sin6_family == AF_INET || sa.sin6_family == PF_INET) {
        struct sockaddr_in *sa4 = (struct sockaddr_in*)&sa;
        port = uint16((uint8_t *)&sa4->sin_port);
        byte_copy(client->ip, (void *)ip6mappedv4prefix, 12);
        byte_copy(client->ip + 12, (void *)&sa4->sin_addr, 4);	
    } else {
        port = uint16((uint8_t *)&sa.sin6_port);
        byte_copy(client->ip, (void *)&sa.sin6_addr, 16);
    }
#else
    uint16_t port = uint16((uint8_t *)&sa.sin_port);
    byte_copy(client->ip, (void *)ip6mappedv4prefix, 12);
    byte_copy(client->ip + 12, (void *)&sa.sin_addr, 4);
#endif
    char host[FORMAT_IP6_LEN];
    byte_zero(host, sizeof host);
    format_ip6(host, client->ip);
    dmr_log_debug("accepted new client connection from %s:%d", host, port);

#if defined(PLATFORM_LINUX) || defined(PLATFORM_OSX)
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) {
        dmr_log_error("fcntl(F_GETFL): %s", strerror(errno));
        goto bail;
    }
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) {
        dmr_log_error("fcntl(F_SETFL): %s", strerror(errno));
        goto bail;
    }
#endif
    return fd;

bail:
    if (client != NULL) {
        del_client(client);
    }
    return -1;
}

int read_client(client_t *client)
{
    dmr_log_debug("client read %llu/%llu", client->bufpos, client->buflen);
    size_t ret = socket_read(client->s, client->buf + client->bufpos,
        client->buflen - client->bufpos);
    switch (ret) {
    case 0:
    case -1:
        if (errno == EINTR || errno == EAGAIN) {
            return 0;
        }
        dmr_log_error("read(): %s", strerror(errno));
        return -1;
    default:
        dmr_log_debug("client sent %llu", ret);
        client->bufpos += ret;
        dmr_log_debug("client done, sending response");
        if (respond_to_client(client) == 0) {
            /* Don't clean up the client struct if there is a file descriptor that
             * contains data-to-be-copied to the client
             */
            if (client->readfd != 0) {
                return 0;
            }
        }
        del_client(client);
        return 1;
    }
}

int send_client(client_t *client)
{
    dmr_log_debug("client send");
    off_t pos = lseek(client->readfd, 0, SEEK_CUR);
    off_t len = lseek(client->readfd, 0, SEEK_END);
    if (pos < len) {
        lseek(client->readfd, pos, SEEK_SET);
        int ret = socket_sendfile(client->s, client->readfd, pos, len);
        if (ret == -1) {
            dmr_log_error("sendfile(): %s", strerror(errno));
        }
    }
    close(client->readfd);
    client->readfd = 0;
    return 1;
}

int start_httpd(void *unused)
{
	DMR_UNUSED(unused);
	config_t *config = load_config();

	dmr_thread_name_set("httpd");

	char host[FORMAT_IP6_LEN];
	uint8_t *ip = config->httpd.bind;
	byte_zero(host, FORMAT_IP6_LEN);
	if ((byte_equal(ip + 12, (uint8_t *)ip4loopback,  4) && isip4mapped(ip)) ||
		(byte_equal(ip +  0, (uint8_t *)ip6loopback, 16))) {
		strncpy(host, localhost, FORMAT_IP6_LEN);
	} else if (isip4mapped(ip)) {
		format_ip4(host, ip + 12);
	} else {
		format_ip6(host, ip);
	}

	dmr_log_info("starting on http://%s:%u/", host, config->httpd.port);
	socket_listen(httpd->s, HTTP_CLIENT_MAX);

    // sendfile() may trigger SIGPIPE
    signal(SIGPIPE, SIG_IGN);

    fd_set rfds, wfds, master, sender;
    FD_ZERO(&rfds);
    FD_ZERO(&wfds);
    FD_ZERO(&master);
    FD_ZERO(&sender);
    int fdmax;
    FD_SET(httpd->s->fd, &master);
    fdmax = httpd->s->fd;

	while (true) {
        memcpy(&rfds, &master, sizeof(master));
        memcpy(&wfds, &sender, sizeof(sender));

        if (select(fdmax + 1, &rfds, &wfds, NULL, NULL) == -1) {
            dmr_log_error("select(): %s", strerror(errno));
            goto bail;
        }

        int i, j, k = 0, l = 0;
        for (i = 0; i <= fdmax; i++) {
            if (FD_ISSET(i, &rfds)) {
                bool handled = false;
                k++;
                if (i == httpd->s->fd) {
                    handled = true;
                    int newfd = accept_client();
                    if (newfd > 0) {
                        FD_SET(newfd, &master);
                        if (newfd > fdmax) {
                            fdmax = newfd;
                        }
                    }
                } else {
                    for (j = 0; j < httpd->clients; j++) {
                        client_t *client = httpd->client[j];
                        if (client != NULL && client->s != NULL && client->s->fd == i) {
                            handled = true;
                            if (read_client(client) != 0) {
                                close(i);
                                FD_CLR(i, &master);
                            }
                            if (client != NULL && client->readfd > 0) {
                                /* Stop reading from the client, start writing */
                                FD_CLR(i, &master);
                                FD_SET(i, &sender);
                            }
                            l++;
                        }
                    }
                }
                dmr_log_debug("%d/%d reads handled", l, k);
                if (!handled) {
                    close(i);
                    FD_CLR(i, &master);
                }
            } else if (FD_ISSET(i, &wfds)) {
                bool handled = false;
                for (j = 0; j < httpd->clients; j++) {
                    client_t *client = httpd->client[j];
                    if (client != NULL && client->s != NULL && client->s->fd == i) {
                        handled = true;
                        if (client->readfd > 0) {
                            if (send_client(client) != 0) {
                                close(i);
                                FD_CLR(i, &sender);
                            }
                            l++;
                        } else {
                            close(i);
                            FD_CLR(i, &sender);
                        }
                    }
                }
                if (!handled) {
                    close(i);
                    FD_CLR(i, &sender);
                }
            }
        }
	}
	goto done;

bail:
	dmr_thread_exit(dmr_thread_error);
	return dmr_thread_error;

done:
	dmr_thread_exit(dmr_thread_success);
    return dmr_thread_success;
}

int init_http(void)
{
	int ret = 0;
	config_t *config = load_config();

	if (!config->httpd.enabled) {
		dmr_log_info("not enabled");
		return ret;
	}

	if ((httpd = talloc_zero(NULL, httpd_t)) == NULL) {
		ret = DMR_OOM();
		goto bail;
	}
	if ((httpd->s = socket_tcp6(0)) == NULL) {
		ret = DMR_OOM();
		goto bail;
	}
	if ((httpd->thread = talloc_zero(httpd, dmr_thread_t)) == NULL) {
		ret = DMR_OOM();
		goto bail;
	}
#if defined(PLATFORM_LINUX) || defined(PLATFORM_OSX)
    if (fcntl(httpd->s->fd, F_SETFL, O_NONBLOCK) == -1) {
        dmr_log_error("fcntl(F_SETFL, O_NONBLOCK): %s", strerror(errno));
        goto bail;
    }
#endif
	int mode = 0;
#if defined(HAVE_LIBC_IPV6) && defined(IPV6_V6ONLY)
	if (setsockopt(httpd->s->fd, IPPROTO_IPV6, IPV6_V6ONLY, (void *)&mode, sizeof mode) != 0) {
		dmr_log_error("setsockopt(IPV6_V6ONLY, %d): %s", mode, strerror(errno));
	}
#endif
#if defined(TCP_NODELAY)
    mode = 1;
    if (setsockopt(httpd->s->fd, IPPROTO_TCP, TCP_NODELAY, (void *)&mode, sizeof mode) != 0) {
        dmr_log_error("setsockopt(TCP_NODELAY, %d): %s", mode, strerror(errno));
    }
#endif
#if defined(SO_REUSEADDR)
	mode = 1;
    if (setsockopt(httpd->s->fd, SOL_SOCKET, SO_REUSEADDR, (void *)&mode, sizeof mode) != 0) {
        dmr_log_error("setsockopt(SO_REUSEADDR, %d): %s", mode, strerror(errno));
    }
#endif
#if defined(SO_REUSEPORT)
    mode = 1;
    if (setsockopt(httpd->s->fd, SOL_SOCKET, SO_REUSEPORT, (void *)&mode, sizeof mode) != 0) {
        dmr_log_error("setsockopt(SO_REUSEPORT, %d): %s", mode, strerror(errno));
    }
#endif
	if ((ret = socket_bind(httpd->s, config->httpd.bind, config->httpd.port)) != 0) {
		char host[FORMAT_IP6_LEN];
		byte_zero(host, sizeof host);
		format_ip6(host, config->httpd.bind);
		dmr_log_error("bind(%s:%u) failed: %s", host, config->httpd.port, strerror(errno));
		goto bail;
	}
	if (dmr_thread_create(httpd->thread, start_httpd, NULL) != dmr_thread_success) {
        dmr_log_error("can't start thread");
        ret = dmr_error(DMR_EINVAL);
        goto bail;
    }

	goto done;

bail:
	TALLOC_FREE(httpd);

done:
	return ret;
}
