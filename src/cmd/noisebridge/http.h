#ifndef _NOISEBRIDGE_HTTP_H
#define _NOISEBRIDGE_HTTP_H

#include <dmr/thread.h>
#include "common/socket.h"
#include "http_parser.h"

#define HTTP_CLIENT_MAX 	128
#define HTTP_REQUEST_MAX 	(80 * 1024)
#define HTTP_RESPONSE_MAX 	HTTP_REQUEST_MAX

typedef struct {
	socket_t                   *s;
	ip6_t 		 		   ip;
	//http_parser  		   *parser;
	dmr_thread_t 		   *thread;
	const char 			   *header;
	char 			   	   *request_path;
	char 			   	   *request_file;
    char                   *buf;
    size_t                 buflen;
    size_t                 bufpos;
	size_t 				   headerlen;
    int                    readfd;
	struct http_parser_url url;
} client_t;

typedef struct {
	ip6_t 	 	 bind;
	uint16_t 	 port;
	socket_t 	 *s;
	dmr_thread_t *thread;
	client_t 	 *client[HTTP_CLIENT_MAX];
	int    		 clients;
} httpd_t;

int init_http(void);

#endif // _NOISEBRIDGE_HTTP_H
