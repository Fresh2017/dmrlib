#ifndef _DMR_IO_H
#define _DMR_IO_H

#include <stdbool.h>
#include <sys/queue.h>
#include <time.h>
#include <dmr/config.h>
#include <dmr/platform.h>

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
    DMR_REQUEST_CLOSE,
    DMR_REQUEST_SHUTDOWN,
    DMR_REQUEST_TIMER,
    DMR_REQUEST_TYPES
} dmr_request_type;

#define REQUEST_FIELDS \
    void *userdata;

/* We could have done with a more generic dmr_io_cb, but that won't allow the
 * compiler to catch wrongly assigned callback types. */
typedef int (*dmr_signal_cb)(void *userdata, int signal);
typedef int (*dmr_connect_cb)(void *userdata, int fd);
typedef int (*dmr_read_cb)(void *userdata, int fd);
typedef int (*dmr_write_cb)(void *userdata, int fd);
typedef int (*dmr_close_cb)(void *userdata, int fd);
typedef int (*dmr_shutdown_cb)(void *userdata, int fd);
typedef int (*dmr_timer_cb)(void *userdata);

typedef struct dmr_io_entry {
    dmr_handle_type          handle;
    int                      fd;
    void                     *cb;
    void                     *userdata;
    bool                     once;
    LIST_ENTRY(dmr_io_entry) entries;
} dmr_io_entry;

typedef struct {
    LIST_HEAD(, dmr_io_entry) head;
} dmr_io_entry_list;

typedef struct dmr_io_timer {
    struct timeval           timeout;    /* timeout when registered */
    struct timeval           wallclock;  /* wall clock time for timeout */
    dmr_timer_cb             cb;
    void                     *userdata;
    bool                     once;
    LIST_ENTRY(dmr_io_timer) entries;
} dmr_io_timer;

typedef struct {
    LIST_HEAD(, dmr_io_timer) head;
} dmr_io_timer_list;

typedef struct {
    dmr_io_entry_list *entry[DMR_REQUEST_TYPES];
    dmr_io_timer_list *timer;
    struct timeval    timeout;
    /* private */
    ssize_t           entries;              /* total number of entries in all lists */
    int               maxfd;                /* highest fd */
    struct timeval    wallclock;            /* wall clock time for io loop */
    fd_set            readers;
    fd_set            writers;
} dmr_io;

extern dmr_io * dmr_io_new(void);
extern int      dmr_io_loop(dmr_io *io);
extern int      dmr_io_reg_signal(dmr_io *io, int signal, dmr_signal_cb cb, void *userdata, bool once);
extern int      dmr_io_reg_read(dmr_io *io, int fd, dmr_read_cb cb, void *userdata, bool once);
extern int      dmr_io_reg_write(dmr_io *io, int fd, dmr_write_cb cb, void *userdata, bool once);
extern int      dmr_io_reg_close(dmr_io *io, int fd, dmr_close_cb cb, void *userdata, bool once);
extern int      dmr_io_reg_timer(dmr_io *io, struct timeval timeout, dmr_timer_cb cb, void *userdata, bool once);

/* Unexport private data */
#undef REQUEST_FIELDS

#endif // _DMR_IO_H
