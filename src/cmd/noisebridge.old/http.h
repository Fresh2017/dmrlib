#ifndef _NOISEBRIDGE_HTTP_H
#define _NOISEBRIDGE_HTTP_H

#include <stdbool.h>
#include <dmr/thread.h>

typedef enum {
    HTTP_OK          = 200,
    HTTP_BAD_REQUEST = 400,
    HTTP_NOT_FOUND   = 404,
    HTTP_ERROR       = 500
} http_status_t;

typedef struct {
    struct sockaddr_in bind;
    int                fd;
    dmr_thread_t       thread;
} http_t;

bool init_http(void);
bool boot_http(void);

#endif // _NOISEBRIDGE_HTTP_H
