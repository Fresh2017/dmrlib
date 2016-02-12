#ifndef _DMR_IO_H
#define _DMR_IO_H

#include <stdbool.h>
#include <time.h>
#include <sys/types.h>
#include <unistd.h>
#include <dmr/config.h>
#include <dmr/platform.h>
#include <dmr/queue.h>

typedef enum {
    DMR_HANDLE_UNKNOWN = 0,
    DMR_HANDLE_TCP,
    DMR_HANDLE_UDP,
    DMR_HANDLE_TTY,
    DMR_HANDLE_FILE,
    DMR_HANDLE_SERIAL,
    DMR_HANDLE_TYPES
} dmr_handle_type;

typedef enum {
    DMR_REQUEST_UNKNOWN = 0,
    DMR_REQUEST_SIGNAL,
    DMR_REQUEST_CONNECT,
    DMR_REQUEST_READ,
    DMR_REQUEST_WRITE,
    DMR_REQUEST_ERROR,
    DMR_REQUEST_TIMER,
    DMR_REQUEST_CLOSE,
    DMR_REQUEST_TYPES
} dmr_request_type;

#define REQUEST_FIELDS \
    void *userdata;

typedef struct dmr_io dmr_io;

/* We could have done with a more generic dmr_io_cb, but that won't allow the
 * compiler to catch wrongly assigned callback types. */
typedef int (*dmr_signal_cb)(dmr_io *io, void *userdata, int signal);
typedef int (*dmr_connect_cb)(dmr_io *io, void *userdata, int fd);
typedef int (*dmr_read_cb)(dmr_io *io, void *userdata, int fd);
typedef int (*dmr_write_cb)(dmr_io *io, void *userdata, int fd);
typedef int (*dmr_error_cb)(dmr_io *io, void *userdata, int fd);
typedef int (*dmr_shutdown_cb)(dmr_io *io, void *userdata, int fd);
typedef int (*dmr_close_cb)(dmr_io *io, void *userdata);
typedef int (*dmr_timer_cb)(dmr_io *io, void *userdata);

typedef struct dmr_io_entry {
    dmr_handle_type              handle;
    int                          fd;
    void                         *cb;
    void                         *userdata;
    bool                         once;
    DMR_LIST_ENTRY(dmr_io_entry) entries;
} dmr_io_entry;

typedef struct {
    DMR_LIST_HEAD(, dmr_io_entry) head;
} dmr_io_entry_list;

typedef struct dmr_io_timer {
    struct timeval               timeout;    /* timeout when registered */
    struct timeval               wallclock;  /* wall clock time for timeout */
    dmr_timer_cb                 cb;
    void                         *userdata;
    bool                         once;
    DMR_LIST_ENTRY(dmr_io_timer) entries;
} dmr_io_timer;

typedef struct {
    DMR_LIST_HEAD(, dmr_io_timer) head;
} dmr_io_timer_list;

struct dmr_io {
    dmr_io_entry_list *entry[DMR_REQUEST_TYPES];
    dmr_io_timer_list *timer;
    struct timeval    timeout;
    /* private */
    ssize_t           entries;              /* total number of entries in all lists */
    int               maxfd;                /* highest fd */
    struct timeval    wallclock;            /* wall clock time for io loop */
    fd_set            readers;
    fd_set            writers;
    fd_set            errors;
    volatile bool     closed;
};

#include <dmr/protocol.h>

extern dmr_io * dmr_io_new(void);
extern int dmr_io_add_protocol(dmr_io *io, dmr_protocol protocol, void *instance);
extern int dmr_io_free(dmr_io *io);
extern int dmr_io_loop(dmr_io *io);
extern int dmr_io_close(dmr_io *io);

extern int dmr_io_reg_signal(dmr_io *io, int signal, dmr_signal_cb cb, void *userdata, bool once);

extern int dmr_io_reg_read(dmr_io *io, int fd, dmr_read_cb cb, void *userdata, bool once);
extern int dmr_io_del_read(dmr_io *io, int fd, dmr_read_cb cb);

extern int dmr_io_reg_write(dmr_io *io, int fd, dmr_write_cb cb, void *userdata, bool once);
extern int dmr_io_del_write(dmr_io *io, int fd, dmr_write_cb cb);

extern int dmr_io_reg_error(dmr_io *io, int fd, dmr_error_cb cb, void *userdata, bool once);
extern int dmr_io_del_error(dmr_io *io, int fd, dmr_error_cb cb);

extern int dmr_io_reg_timer(dmr_io *io, struct timeval timeout, dmr_timer_cb cb, void *userdata, bool once);
extern int dmr_io_del_timer(dmr_io *io, dmr_timer_cb cb);

extern int dmr_io_reg_close(dmr_io *io, dmr_close_cb cb, void *userdata);

/* Unexport private data */
#undef REQUEST_FIELDS

#endif // _DMR_IO_H
