#include <talloc.h>
#include <sys/select.h>
#include <sys/time.h>
#include "dmr/error.h"
#include "dmr/io.h"
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
        if ((io->entry[i] = talloc_zero(io, dmr_io_entry_list)) == NULL) {
            TALLOC_FREE(io);
            dmr_error(DMR_ENOMEM);
            return NULL;
        }
        LIST_INIT(&io->entry[i]->head);
    }

    FD_ZERO(&io->readers);
    FD_ZERO(&io->writers);

    return io;
}

DMR_PRV int io_handle_readable(dmr_io *io, int fd)
{
    dmr_io_entry *entry, *next;
    LIST_FOREACH_SAFE(entry, &io->entry[DMR_REQUEST_READ]->head, entries, next) {
        if (entry->fd == fd) {
            ((dmr_read_cb)entry->cb)(entry->userdata, fd);
            if (entry->once) {
                LIST_REMOVE(entry, entries);
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
    LIST_FOREACH_SAFE(entry, &io->entry[DMR_REQUEST_WRITE]->head, entries, next) {
        if (entry->fd == fd) {
            ((dmr_write_cb)entry->cb)(entry->userdata, fd);
            if (entry->once) {
                LIST_REMOVE(entry, entries);
                io->entries--;
                TALLOC_FREE(entry);
                FD_CLR(fd, &io->writers);
            }
            return 1;
        }
    }

    return 0;
}

DMR_PRV int io_handle_signal(dmr_io *io, int sig)
{
    dmr_io_entry *entry, *next;
    LIST_FOREACH_SAFE(entry, &io->entry[DMR_REQUEST_SIGNAL]->head, entries, next) {
        if (entry->fd == sig) {
            ((dmr_signal_cb)entry->cb)(entry->userdata, sig);
            if (entry->once) {
                LIST_REMOVE(entry, entries);
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
    SLIST_ENTRY(io_sig) entries;
} io_sig;

static SLIST_HEAD(, io_sig) io_sig_head;

/** Add a signal handler callback
 * Registers io loop signal to be handled, reg indicates if the signal was
 * previously registered */
DMR_PRV int io_sig_add(dmr_io *io, int sig, bool *reg)
{
    io_sig *entry, *next;
    bool found = false;
    SLIST_FOREACH_SAFE(entry, &io_sig_head, entries, next) {
        if (*reg && entry->sig == sig) {
            *reg = false;
        }
        if ((found = (entry->io == io && entry->sig == sig)))
            break;
    }
    if (!found) {
        if ((entry = talloc_zero(NULL, io_sig)) == NULL)
            return dmr_error(DMR_ENOMEM);
        entry->io = io;
        entry->sig = sig;
        SLIST_INSERT_HEAD(&io_sig_head, entry, entries); 
    }
    return 0;
}

/** Signal dispatcher */
DMR_PRV void io_sig_handle(int sig)
{
    io_sig *entry, *next;
    SLIST_FOREACH_SAFE(entry, &io_sig_head, entries, next) {
        if (entry->sig == sig) {
            if (io_handle_signal(entry->io, sig) != 0) {
                break;
            }
        }
    }
}

/** Timer calculation */
DMR_PRV void io_timeout(dmr_io *io, struct timeval *timeout)
{
    /* Default timeout */
    byte_copy(timeout, &io->timeout, sizeof(struct timeval));

    /* Timeout for timers */
    dmr_io_timer *timer, *next;
    struct timeval delta;
    LIST_FOREACH_SAFE(timer, &io->timer->head, entries, next) {
        timersub(&timer->wallclock, &io->wallclock, &delta);
        if (!timercmp(&delta, timeout, <)) {
            byte_copy(timeout, &delta, sizeof(struct timeval));
        }
    }
}

/* Run callbacks for timers that have expired */
DMR_PRV void io_handle_timers(dmr_io *io)
{
    dmr_io_timer *timer, *next;
    LIST_FOREACH_SAFE(timer, &io->timer->head, entries, next) {
        if (!timercmp(&timer->wallclock, &io->wallclock, >))
            continue;

        timer->cb(timer->userdata);
        if (timer->once) {
            LIST_REMOVE(timer, entries);
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
    struct timeval timeout;
    while (io->entries > 0) {
        fd_set rfds, wfds;
        byte_copy(&rfds, &io->readers, sizeof rfds);
        byte_copy(&wfds, &io->writers, sizeof wfds);
        gettimeofday(&io->wallclock, NULL);

        do {
            io_timeout(io, &timeout);
            ret = select(io->maxfd + 1, &rfds, &wfds, NULL, &timeout);
        } while (ret == -1 && (errno == EAGAIN || errno == EINTR));

        io_handle_timers(io);
        for (i = 0; i < io->maxfd; i++) {
            if (FD_ISSET(i, &rfds)) {
                io_handle_readable(io, i);
            }
            if (FD_ISSET(i, &wfds)) {
                io_handle_writable(io, i);
            }
        }
    }

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
        signal(sig, io_sig_handle);
    }

    e->handle = DMR_HANDLE_UNKNOWN;
    e->fd = sig;
    e->cb = cb;
    e->userdata = userdata;
    e->once = once;
    LIST_INSERT_HEAD(&io->entry[DMR_REQUEST_SIGNAL]->head, e, entries);
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
    LIST_INSERT_HEAD(&io->entry[DMR_REQUEST_READ]->head, e, entries);
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
    LIST_INSERT_HEAD(&io->entry[DMR_REQUEST_WRITE]->head, e, entries);
    io->entries++;
    FD_SET(fd, &io->writers);
    io->maxfd = MAX(io->maxfd, fd);
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

    byte_copy(&t->timeout, &timeout, sizeof timeout);
    timeradd(&io->wallclock, &t->timeout, &t->wallclock);
    t->cb = cb;
    t->userdata = userdata;
    t->once = once;
    LIST_INSERT_HEAD(&io->timer->head, t, entries);
    io->entries++;
    return 0;
}
