#include <talloc.h>
#include "dmr/config.h"
#include "dmr/error.h"
#include "dmr/packetq.h"
#include "common/byte.h"

DMR_API dmr_packetq *dmr_packetq_new(void)
{
    dmr_packetq *q = talloc_zero(NULL, dmr_packetq);
    if (q == NULL) {
        return NULL;
    }
    STAILQ_INIT(&q->head);
    return q;    
}

DMR_API int dmr_packetq_add(dmr_packetq *q, dmr_parsed_packet *parsed)
{
    if (q == NULL || parsed == NULL)
        return dmr_error(DMR_EINVAL);

    dmr_packetq_entry *e = talloc_zero(q, dmr_packetq_entry);
    if (e == NULL) {
        return DMR_OOM();
    }

    e->parsed = parsed;
    STAILQ_INSERT_TAIL(&q->head, e, entries);

    return 0;
}

DMR_API int dmr_packetq_add_packet(dmr_packetq *q, dmr_packet packet)
{
    if (q == NULL || packet == NULL)
        return dmr_error(DMR_EINVAL);
    
    dmr_parsed_packet *parsed = talloc_zero(NULL, dmr_parsed_packet);
    if (parsed == NULL) {
        return DMR_OOM();
    }

    byte_copy(parsed->packet, packet, sizeof(dmr_packet));
    parsed->parsed = false;

    return dmr_packetq_add(q, parsed);
}

DMR_API int dmr_packetq_shift(dmr_packetq *q, dmr_parsed_packet **parsed_out)
{
    if (q == NULL)
        return dmr_error(DMR_EINVAL);

    if (STAILQ_EMPTY(&q->head))
        return -1;

    dmr_packetq_entry *e = STAILQ_FIRST(&q->head);
    STAILQ_REMOVE_HEAD(&q->head, entries);
    dmr_parsed_packet *parsed = e->parsed;
    TALLOC_FREE(e);

    *parsed_out = parsed;
    return 0;
}

int dmr_packetq_shift_packet(dmr_packetq *q, dmr_packet packet_out)
{
    dmr_parsed_packet *parsed;
    int ret;

    if ((ret = dmr_packetq_shift(q, &parsed)) != 0)
        return ret;

    byte_copy(packet_out, parsed->packet, sizeof(dmr_packet));

    TALLOC_FREE(parsed);
    return 0;
}

int dmr_packetq_flush(dmr_packetq *q)
{
    if (q == NULL)
        return dmr_error(DMR_EINVAL);

    dmr_packetq_entry *entry, *next;
    STAILQ_FOREACH_SAFE(entry, &q->head, entries, next) {
        STAILQ_REMOVE(&q->head, entry, dmr_packetq_entry, entries);
        TALLOC_FREE(entry->parsed);
        TALLOC_FREE(entry);
    }

    return 0;
}

DMR_API int dmr_packetq_foreach(dmr_packetq *q, dmr_parsed_packet_cb cb, void *userdata)
{
    if (q == NULL)
        return dmr_error(DMR_EINVAL);

    if (STAILQ_EMPTY(&q->head))
        return 0;

    dmr_packetq_entry *entry, *next;
    int ret;
    STAILQ_FOREACH_SAFE(entry, &q->head, entries, next) {
        if ((ret = cb(entry->parsed, userdata)) != 0)
            return ret;
    }
    return 0;
}

DMR_API int dmr_packetq_foreach_packet(dmr_packetq *q, dmr_packet_cb cb, void *userdata)
{
    if (q == NULL)
        return dmr_error(DMR_EINVAL);

    if (STAILQ_EMPTY(&q->head))
        return 0;

    dmr_packetq_entry *entry, *next;
    int ret;
    STAILQ_FOREACH_SAFE(entry, &q->head, entries, next) {
        if ((ret = cb(entry->parsed->packet, userdata)) != 0)
            return ret;
    }
    return 0;
}
