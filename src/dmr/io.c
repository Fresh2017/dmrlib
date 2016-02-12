#include <talloc.h>
#include <signal.h>
#include <sys/select.h>
#include <sys/time.h>
#include "dmr/error.h"
#include "dmr/io.h"
#include "dmr/malloc.h"
#include "common/byte.h"

DMR_API dmr_io *dmr_io_new(void)
{
    dmr_io *io;
    int i;

    if ((io = talloc_zero(NULL, dmr_io)) == NULL) {
        dmr_error(DMR_ENOMEM);
        return NULL;
    }
    for (i = 0; i < DMR_REQUEST_TYPES; i++) {
        if ((io->entry[i] = dmr_palloc(io, dmr_io_entry_list)) == NULL) {
            dmr_free(io);
            dmr_error(DMR_ENOMEM);
            return NULL;
        }
        DMR_LIST_INIT(&io->entry[i]->head);
    }
    if ((io->timer = dmr_palloc(io, dmr_io_timer_list)) == NULL) {
        dmr_free(io);
        dmr_error(DMR_ENOMEM);
        return NULL;
    }
    DMR_LIST_INIT(&io->timer->head);

    FD_ZERO(&io->readers);
    FD_ZERO(&io->writers);
    FD_ZERO(&io->errors);

    gettimeofday(&io->wallclock, NULL);

    return io;
}

DMR_API int dmr_io_add_protocol(dmr_io *io, dmr_protocol protocol, void *instance)
{
    DMR_ERROR_IF_NULL(io, DMR_EINVAL);
    int ret;

    dmr_log_debug("io: add protocol %s", protocol.name);

    if (protocol.init_io == NULL) {
        dmr_log_error("io: protocol has no init_io callback");
        return dmr_error(DMR_EINVAL);
    }
    if (protocol.register_io == NULL) {
        dmr_log_error("io: protocol has no register_io callback");
        return dmr_error(DMR_EINVAL);
    }

    if ((ret = protocol.init_io(io, instance)) == 0) {
        if ((ret = protocol.register_io(io, instance)) != 0) {
            dmr_log_error("io: protocol register failed: %s", dmr_error_get());
        }
    } else {
        dmr_log_error("io: protocol init failed: %s", dmr_error_get());
    }
    return ret;
}

DMR_PRV int io_handle_readable(dmr_io *io, int fd)
{
    dmr_io_entry *entry, *next;
    DMR_LIST_FOREACH_SAFE(entry, &io->entry[DMR_REQUEST_READ]->head, entries, next) {
        if (entry->fd == fd) {
            ((dmr_read_cb)entry->cb)(io, entry->userdata, fd);
            if (entry->once) {
                DMR_LIST_REMOVE(entry, entries);
                io->entries--;
                TALLOC_FREE(entry);
                FD_CLR(fd, &io->readers);
            }
        }
    }

    return 0;
}

DMR_PRV int io_handle_writable(dmr_io *io, int fd)
{
    dmr_io_entry *entry, *next;
    DMR_LIST_FOREACH_SAFE(entry, &io->entry[DMR_REQUEST_WRITE]->head, entries, next) {
        if (entry->fd == fd) {
            ((dmr_write_cb)entry->cb)(io, entry->userdata, fd);
            if (entry->once) {
                DMR_LIST_REMOVE(entry, entries);
                io->entries--;
                TALLOC_FREE(entry);
                FD_CLR(fd, &io->writers);
            }
            return 1;
        }
    }

    return 0;
}

DMR_PRV int io_handle_error(dmr_io *io, int fd)
{
    dmr_io_entry *entry, *next;
    DMR_LIST_FOREACH_SAFE(entry, &io->entry[DMR_REQUEST_ERROR]->head, entries, next) {
        if (entry->fd == fd) {
            ((dmr_error_cb)entry->cb)(io, entry->userdata, fd);
            if (entry->once) {
                DMR_LIST_REMOVE(entry, entries);
                io->entries--;
                TALLOC_FREE(entry);
                FD_CLR(fd, &io->writers);
            }
            return 1;
        }
    }

    return 0;
}

DMR_PRV int io_handle_close(dmr_io *io)
{
    dmr_io_entry *entry, *next;
    DMR_LIST_FOREACH_SAFE(entry, &io->entry[DMR_REQUEST_CLOSE]->head, entries, next) {
        ((dmr_close_cb)entry->cb)(io, entry->userdata);
        DMR_LIST_REMOVE(entry, entries);
        TALLOC_FREE(entry);
    }

    return 0;
}

DMR_PRV int io_handle_signal(dmr_io *io, int sig)
{
    dmr_io_entry *entry, *next;
    DMR_LIST_FOREACH_SAFE(entry, &io->entry[DMR_REQUEST_SIGNAL]->head, entries, next) {
        if (entry->fd == sig) {
            ((dmr_signal_cb)entry->cb)(io, entry->userdata, sig);
            if (entry->once) {
                DMR_LIST_REMOVE(entry, entries);
                io->entries--;
                TALLOC_FREE(entry);
            }
            return 1;
        }
    }

    return 0;
}

typedef struct io_sig {
    dmr_io              *io;
    int                 sig;
    DMR_LIST_ENTRY(io_sig) entries;
} io_sig;

static DMR_LIST_HEAD(, io_sig) io_sig_head;

/** Add a signal handler callback
 * Registers io loop signal to be handled, reg indicates if the signal was
 * previously registered */
DMR_PRV int io_sig_add(dmr_io *io, int sig, bool *reg)
{
    io_sig *entry, *next;
    bool found = false;
    DMR_LIST_FOREACH_SAFE(entry, &io_sig_head, entries, next) {
        if (entry->sig == sig) {
            found = true;
        }
        if ((found = (entry->io == io && entry->sig == sig)))
            break;
    }
    if (!found) {
        if ((entry = talloc_zero(NULL, io_sig)) == NULL)
            return dmr_error(DMR_ENOMEM);
        entry->io = io;
        entry->sig = sig;
        DMR_LIST_INSERT_HEAD(&io_sig_head, entry, entries); 
    }
    *reg = !found;
    return 0;
}

/** Signal dispatcher */
DMR_PRV void io_sig_handle(int sig)
{
    io_sig *entry, *next;
#if defined(DMR_DEBUG)
    size_t i = 0;
    DMR_LIST_FOREACH_SAFE(entry, &io_sig_head, entries, next) {
        i++;
    }
    dmr_log_debug("io: received signal %d, running %zu callbacks", sig, i);
#endif
    DMR_LIST_FOREACH_SAFE(entry, &io_sig_head, entries, next) {
        if (entry->sig == sig) {
            if (io_handle_signal(entry->io, sig) != 0) {
                break;
            }
        }
    }
}

/** Timer calculation */
DMR_PRV int io_timeout(dmr_io *io, struct timeval *timeout)
{
    int timers = 0;

    /* Default timeout */
    byte_copy(timeout, &io->timeout, sizeof(struct timeval));

    if (DMR_LIST_EMPTY(&io->timer->head))
        return 0;

    /* Timeout for timers */
    dmr_io_timer *timer, *next;
    struct timeval delta;

    /* We compare the timers with both our default timeout and the
     * delta between current wall clock and timer wall clock to find out what
     * the smallest required timeout is. */
    DMR_LIST_FOREACH_SAFE(timer, &io->timer->head, entries, next) {
        timersub(&timer->wallclock, &io->wallclock, &delta);
        if (!timercmp(&delta, timeout, <)) {
            /* Timer timeout is smaller than current timeout. */
            byte_copy(timeout, &delta, sizeof(struct timeval));
        }
        timers++;
    }

    return timers;
}

/* Run callbacks for timers that have expired */
DMR_PRV void io_handle_timers(dmr_io *io)
{
    dmr_io_timer *timer, *next;

    /* Check for each of the timers if they are past current wall clock time,
     * if so, execute their callback. */
    DMR_LIST_FOREACH_SAFE(timer, &io->timer->head, entries, next) {
        if (!timercmp(&timer->wallclock, &io->wallclock, <))
            continue;

        timer->cb(io, timer->userdata);
        if (timer->once) {
            DMR_LIST_REMOVE(timer, entries);
            io->entries--;
            TALLOC_FREE(timer);
        } else {
            /* Update next timer wakeup */
            timeradd(&io->wallclock, &timer->timeout, &timer->wallclock);
        }
    }
}

DMR_API int dmr_io_loop(dmr_io *io)
{
    if (io == NULL)
        return dmr_error(DMR_EINVAL);

    int ret, i;
    struct timeval *timeout = dmr_palloc(io, struct timeval);
    if (timeout == NULL)
        return dmr_error(DMR_ENOMEM);

    io->closed = false;
    while (io->entries > 0 && !io->closed) {
        fd_set rfds, wfds, efds;
        byte_copy(&rfds, &io->readers, sizeof rfds);
        byte_copy(&wfds, &io->writers, sizeof wfds);
        byte_copy(&efds, &io->errors,  sizeof efds);
        gettimeofday(&io->wallclock, NULL);

        do {
            if (io_timeout(io, timeout))
                ret = select(io->maxfd + 1, &rfds, &wfds, &efds, timeout);
            else
                ret = select(io->maxfd + 1, &rfds, &wfds, &efds, NULL);
        } while (ret == -1 && (errno == EAGAIN || errno == EINTR));

        io_handle_timers(io);
        for (i = 0; i < io->maxfd; i++) {
            if (FD_ISSET(i, &efds)) {
                io_handle_error(io, i);
            }
            if (FD_ISSET(i, &rfds)) {
                io_handle_readable(io, i);
            }
            if (FD_ISSET(i, &wfds)) {
                io_handle_writable(io, i);
            }
        }
    }

    return io_handle_close(io);
}

DMR_API int dmr_io_close(dmr_io *io)
{
    DMR_ERROR_IF_NULL(io, DMR_EINVAL);
    
    io->closed = true;
    return 0;
}

DMR_API int dmr_io_free(dmr_io *io)
{
    if (io == NULL)
        return 0;

    dmr_request_type i;
    dmr_io_entry *entry, *enext;
    for (i = 0; i < DMR_REQUEST_TYPES; i++) {
        DMR_LIST_FOREACH_SAFE(entry, &io->entry[i]->head, entries, enext) {
            dmr_free(entry);
        }
    }
    dmr_io_timer *timer, *tnext;
    DMR_LIST_FOREACH_SAFE(timer, &io->timer->head, entries, tnext) {
        dmr_free(timer);
    }
    dmr_free(io);
    return 0;
}

DMR_API int dmr_io_reg_signal(dmr_io *io, int sig, dmr_signal_cb cb, void *userdata, bool once)
{
    if (io == NULL)
        return dmr_error(DMR_EINVAL);

    dmr_io_entry *e;
    if ((e = talloc_zero(io, dmr_io_entry)) == NULL) {
        return dmr_error(DMR_ENOMEM);
    }

    int ret = 0;
    bool reg = false;
    if ((ret = io_sig_add(io, sig, &reg)) != 0)
        return dmr_error(DMR_LASTERROR);
    
    if (reg) {
        dmr_log_debug("io: registering signal handler for %d", sig);
        signal(sig, io_sig_handle);
    }

    e->handle = DMR_HANDLE_UNKNOWN;
    e->fd = sig;
    e->cb = cb;
    e->userdata = userdata;
    e->once = once;
    DMR_LIST_INSERT_HEAD(&io->entry[DMR_REQUEST_SIGNAL]->head, e, entries);
    io->entries++;
    return 0;
}

DMR_API int dmr_io_reg_read(dmr_io *io, int fd, dmr_read_cb cb, void *userdata, bool once)
{
    if (io == NULL)
        return dmr_error(DMR_EINVAL);

    dmr_io_entry *e;
    if ((e = talloc_zero(io, dmr_io_entry)) == NULL) {
        return dmr_error(DMR_ENOMEM);
    }

    e->handle = DMR_HANDLE_UNKNOWN;
    e->fd = fd;
    e->cb = cb;
    e->userdata = userdata;
    e->once = once;
    DMR_LIST_INSERT_HEAD(&io->entry[DMR_REQUEST_READ]->head, e, entries);
    io->entries++;
    FD_SET(fd, &io->readers);
    io->maxfd = MAX(io->maxfd, fd);
    return 0;
}

DMR_API int dmr_io_reg_write(dmr_io *io, int fd, dmr_write_cb cb, void *userdata, bool once)
{
    if (io == NULL)
        return dmr_error(DMR_EINVAL);

    dmr_io_entry *e;
    if ((e = talloc_zero(io, dmr_io_entry)) == NULL) {
        return dmr_error(DMR_ENOMEM);
    }

    e->handle = DMR_HANDLE_UNKNOWN;
    e->fd = fd;
    e->cb = cb;
    e->userdata = userdata;
    e->once = once;
    DMR_LIST_INSERT_HEAD(&io->entry[DMR_REQUEST_WRITE]->head, e, entries);
    io->entries++;
    FD_SET(fd, &io->writers);
    io->maxfd = MAX(io->maxfd, fd);
    return 0;
}

DMR_API int dmr_io_reg_error(dmr_io *io, int fd, dmr_error_cb cb, void *userdata, bool once)
{
    if (io == NULL)
        return dmr_error(DMR_EINVAL);

    dmr_io_entry *e;
    if ((e = talloc_zero(io, dmr_io_entry)) == NULL) {
        return dmr_error(DMR_ENOMEM);
    }

    e->handle = DMR_HANDLE_UNKNOWN;
    e->fd = fd;
    e->cb = cb;
    e->userdata = userdata;
    e->once = once;
    DMR_LIST_INSERT_HEAD(&io->entry[DMR_REQUEST_ERROR]->head, e, entries);
    io->entries++;
    FD_SET(fd, &io->errors);
    io->maxfd = MAX(io->maxfd, fd);
    return 0;
}

DMR_API int dmr_io_reg_close(dmr_io *io, dmr_close_cb cb, void *userdata)
{
    if (io == NULL)
        return dmr_error(DMR_EINVAL);

    dmr_io_entry *e;
    if ((e = talloc_zero(io, dmr_io_entry)) == NULL) {
        return dmr_error(DMR_ENOMEM);
    }

    e->handle = DMR_HANDLE_UNKNOWN;
    e->fd = 0;
    e->cb = cb;
    e->userdata = userdata;
    e->once = true;
    DMR_LIST_INSERT_HEAD(&io->entry[DMR_REQUEST_CLOSE]->head, e, entries);
    return 0;
}

DMR_API int dmr_io_reg_timer(dmr_io *io, struct timeval timeout, dmr_timer_cb cb, void *userdata, bool once)
{
    if (io == NULL)
        return dmr_error(DMR_EINVAL);

    dmr_io_timer *t;
    if ((t = talloc_zero(io, dmr_io_timer)) == NULL) {
        return dmr_error(DMR_ENOMEM);
    }

    dmr_log_debug("io: register timer interval %ld.%06lds",
        timeout.tv_sec, timeout.tv_usec);

    byte_copy(&t->timeout, &timeout, sizeof timeout);
    timeradd(&io->wallclock, &t->timeout, &t->wallclock);
    t->cb = cb;
    t->userdata = userdata;
    t->once = once;
    DMR_LIST_INSERT_HEAD(&io->timer->head, t, entries);
    io->entries++;
    return 0;
}
