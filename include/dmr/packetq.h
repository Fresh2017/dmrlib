#ifndef _DMR_PACKETQ_H
#define _DMR_PACKETQ_H

#include <dmr/packet.h>
#include <dmr/queue.h>

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct dmr_packetq_entry {
    dmr_parsed_packet                  *parsed;
    DMR_TAILQ_ENTRY(dmr_packetq_entry) entries;
} dmr_packetq_entry;

typedef struct {
    DMR_TAILQ_HEAD(, dmr_packetq_entry) head;
} dmr_packetq;

extern dmr_packetq * dmr_packetq_new(void);
extern int dmr_packetq_add(dmr_packetq *q, dmr_parsed_packet *parsed);
extern int dmr_packetq_add_packet(dmr_packetq *q, dmr_packet packet);
extern int dmr_packetq_shift(dmr_packetq *q, dmr_parsed_packet **parsed_out);
extern int dmr_packetq_shift_packet(dmr_packetq *q, dmr_packet packet_out);
extern int dmr_packetq_foreach(dmr_packetq *q, dmr_parsed_packet_cb cb, void *userdata);
extern int dmr_packetq_foreach_packet(dmr_packetq *q, dmr_packet_cb cb, void *userdata);

#if defined(__cplusplus)
}
#endif

#endif
