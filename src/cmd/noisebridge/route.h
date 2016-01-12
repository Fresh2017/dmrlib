#ifndef _NOISETERM_ROUTE_H
#define _NOISETERM_ROUTE_H

#define ROUTE_RULE_MAX 64

typedef enum {
    ROUTE_PERMIT,
    ROUTE_REJECT
} route_policy_t;

// route_rule=from_mmdvm_modem:{mmdvm,mmdvm,reject}
// route_rule=from_mmdvm_modem:{mmdvm,*,1,2042214,9990}
typedef struct {
    char  *name;
    char  *src_proto;
    char  *dst_proto;
    dmr_ts_t    ts;
    dmr_id_t    src_id;
    dmr_id_t    dst_id;
    route_policy_t policy;
} route_rule_t;

route_rule_t *route_rule_parse(char *line);
bool route_rule_packet(dmr_repeater_t *repeater, dmr_proto_t *src_proto, dmr_proto_t *dst_proto, dmr_packet_t *packet);

#endif // _NOISETERM_ROUTE_H
